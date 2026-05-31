#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Inventory/TradeComponent.h"
#include "Inventory/InventoryComponent.h"
#include "UI/HUD/HUDWidget.h"
#include "UI/Dialogue/DialogueWidget.h"
#include "UI/Shop/ShopScreenWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Interaction/InteractionDetectorComponent.h"
#include "Interaction/InteractionPromptComponent.h"
#include "Interaction/InteractableInterface.h"
#include "UI/Common/InteractionPromptWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"

namespace
{
    // 디버그: DT_ItemData의 모든 아이템을 인벤토리에 지급 (서버에서 호출)
    void DebugGiveAllItems(APlayerCharacter* PC)
    {
        if (PC == nullptr)
        {
            return;
        }

        UInventoryComponent* Inv = PC->GetInventoryComponent();
        if (Inv == nullptr)
        {
            return;
        }

        UDataTable* DT = LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/DT_ItemData.DT_ItemData"));
        if (DT == nullptr)
        {
            return;
        }

        for (const FName& RowName : DT->GetRowNames())
        {
            const FItemData* Row = DT->FindRow<FItemData>(RowName, TEXT("DebugGiveAll"));
            if (Row == nullptr)
            {
                continue;
            }
            if (Inv->IsFull())
            {
                break;
            }
            Inv->AddItem(Row->ItemID, 1);
        }
    }
}

APlayerCharacter::APlayerCharacter()
{
    // SpringArm
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.f;
    CameraBoom->bUsePawnControlRotation = true;

    // Camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // 캐릭터 회전 설정
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);

    // TradeComponent
    TradeComp = CreateDefaultSubobject<UTradeComponent>(TEXT("TradeComp"));

    // InteractionDetector — 범위 내 IInteractable 감지
    InteractionDetector = CreateDefaultSubobject<UInteractionDetectorComponent>(TEXT("InteractionDetector"));
    InteractionDetector->SetupAttachment(RootComponent);

    // 임시 테스트용 강화 화면 단축키(K) 기본값 — 에디터에서 재지정 가능
    static ConstructorHelpers::FObjectFinder<UInputAction> EnhanceIAFinder(
        TEXT("/Game/Input/Actions/IA_Enhance.IA_Enhance"));
    if (EnhanceIAFinder.Succeeded()) EnhanceAction = EnhanceIAFinder.Object;

    static ConstructorHelpers::FClassFinder<UUserWidget> EnhanceWBPFinder(
        TEXT("/Game/UI/Enhance/WBP_EnhanceScreenWidget"));
    if (EnhanceWBPFinder.Succeeded()) EnhanceScreenWidgetClass = EnhanceWBPFinder.Class;

    static ConstructorHelpers::FClassFinder<UUserWidget> ShopWBPFinder(
        TEXT("/Game/UI/Shop/WBP_ShopScreenWidget"));
    if (ShopWBPFinder.Succeeded()) ShopScreenWidgetClass = ShopWBPFinder.Class;

    static ConstructorHelpers::FClassFinder<UUserWidget> PromptWBPFinder(
        TEXT("/Game/UI/Common/WBP_InteractionPrompt"));
    if (PromptWBPFinder.Succeeded()) InteractionPromptClass = PromptWBPFinder.Class;

    PrimaryActorTick.bCanEverTick = true;   // 프롬프트 위치 추적용
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    // Enhanced Input — DefaultMappingContext 등록
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
    {
        if (DefaultMappingContext)
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
    }

    // HUD 스폰 (로컬 플레이어만)
    if (IsLocallyControlled() && HUDWidgetClass)
    {
        HUDWidget = CreateWidget<UHUDWidget>(PC, HUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport();
        }
    }

    // 포커스 변경 이벤트 — 상호작용 프롬프트 표시/숨기기
    if (IsLocallyControlled())
    {
        InteractionDetector->OnFocusChanged.AddDynamic(this, &APlayerCharacter::OnFocusChanged);
    }
}

void APlayerCharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();

    // 컨트롤러 변경 시 IMC 재등록
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->ClearAllMappings();
            if (DefaultMappingContext)
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
    DebugDropItem(0);
}

void APlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateInteractionPrompt();
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

    if (JumpAction)
    {
        EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &APlayerCharacter::HandleJump);
        EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::HandleStopJump);
    }
    if (MoveAction)
    {
        EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::HandleMove);
    }
    if (LookAction)
    {
        EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::HandleLook);
    }
    if (InteractAction)
    {
        EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleInteract);
    }
    if (InventoryAction)
    {
        EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleInventory);
    }
    if (PauseAction)
    {
        EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &APlayerCharacter::HandlePause);
    }
    if (SprintAction)
    {
        EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleSprintStart);
        EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::HandleSprintEnd);
    }
    if (UsePotionAction)
    {
        EIC->BindAction(UsePotionAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleUsePotion);
    }
    if (EnhanceAction)
    {
        EIC->BindAction(EnhanceAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleEnhance);
    }
}

// ── 입력 핸들러 ──────────────────────────────────────────────────────────────

void APlayerCharacter::HandleMove(const FInputActionValue& Value)
{
    const FVector2D MoveVec = Value.Get<FVector2D>();
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MoveVec.Y);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MoveVec.X);
    }
}

void APlayerCharacter::HandleLook(const FInputActionValue& Value)
{
    const FVector2D LookVec = Value.Get<FVector2D>();
    AddControllerYawInput(LookVec.X);
    AddControllerPitchInput(-LookVec.Y);
}

void APlayerCharacter::HandleJump()
{
    Jump();
}

void APlayerCharacter::HandleStopJump()
{
    StopJumping();
}

void APlayerCharacter::HandleInteract()
{
    InteractionDetector->TryInteract();
}

void APlayerCharacter::HandleUsePotion()
{
    // 인벤에서 HP 포션(ItemType==Consumable) 첫 번째 슬롯을 찾아 사용
    // 2단계 GAS UseItem GA로 대체 예정 — 현재는 빈 훅
    if (UInventoryComponent* Inv = FindComponentByClass<UInventoryComponent>())
    {
        // Inv->UseFirstPotion();  // 2단계에서 구현
    }
}

void APlayerCharacter::OnFocusChanged(AActor* NewFocus)
{
    LastFocusedActor = NewFocus;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (NewFocus == nullptr || PC == nullptr)
    {
        if (InteractionPromptW) InteractionPromptW->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (NewFocus->Implements<UInteractable>())
    {
        const FText PromptText = IInteractable::Execute_GetInteractionPrompt(NewFocus);

        if (InteractionPromptW == nullptr && InteractionPromptClass)
        {
            InteractionPromptW = CreateWidget<UUserWidget>(PC, InteractionPromptClass);
            if (InteractionPromptW) InteractionPromptW->AddToViewport(50);
        }
        if (UInteractionPromptWidget* W = Cast<UInteractionPromptWidget>(InteractionPromptW))
        {
            W->SetPromptText(PromptText);
        }
        if (InteractionPromptW)
        {
            InteractionPromptW->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        UpdateInteractionPrompt();
    }
    else if (InteractionPromptW)
    {
        InteractionPromptW->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void APlayerCharacter::UpdateInteractionPrompt()
{
    if (InteractionPromptW == nullptr || LastFocusedActor == nullptr) return;
    if (InteractionPromptW->GetVisibility() == ESlateVisibility::Collapsed) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC == nullptr) return;

    const FVector WorldPos = LastFocusedActor->GetActorLocation() + FVector(0.f, 0.f, 90.f);
    FVector2D Screen;
    if (PC->ProjectWorldLocationToScreen(WorldPos, Screen, false))
    {
        // PromptText가 위젯 내부에서 중앙 정렬이라 투영 지점에 그대로 배치 (오프셋 불필요)
        const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
        const FVector2D Logical = (DPI > 0.f) ? (Screen / DPI) : Screen;
        InteractionPromptW->SetPositionInViewport(Logical, false);
    }
}

void APlayerCharacter::HandleInventory()
{
    if (bInventoryOpen)
    {
        CloseInventory();
    }
    else
    {
        OpenInventory();
    }
}

void APlayerCharacter::HandlePause()
{
    if (bPauseMenuOpen)
    {
        ClosePauseMenu();
    }
    else
    {
        OpenPauseMenu();
    }
}

void APlayerCharacter::HandleEnhance()
{
    if (bEnhanceOpen)
    {
        CloseEnhance();
    }
    else
    {
        OpenEnhance();
    }
}

void APlayerCharacter::HandleSprintStart()
{
    bIsSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = 600.f;
}

void APlayerCharacter::HandleSprintEnd()
{
    bIsSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = 300.f;
}

// ── UI 열기/닫기 ─────────────────────────────────────────────────────────────

void APlayerCharacter::OpenInventory()
{
    if (bInventoryOpen) return;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC == nullptr) return;

    if (InventoryScreenWidget == nullptr && InventoryScreenWidgetClass)
    {
        InventoryScreenWidget = CreateWidget<UUserWidget>(PC, InventoryScreenWidgetClass);
    }
    if (InventoryScreenWidget)
    {
        InventoryScreenWidget->AddToViewport();
        if (UButton* CloseB = Cast<UButton>(InventoryScreenWidget->GetWidgetFromName(TEXT("CloseBtn"))))
            CloseB->OnClicked.AddUniqueDynamic(this, &APlayerCharacter::CloseInventory);
        bInventoryOpen = true;
        SwitchToUIInput();
    }
}

void APlayerCharacter::CloseInventory()
{
    if (bInventoryOpen == false) return;
    if (InventoryScreenWidget)
    {
        InventoryScreenWidget->RemoveFromParent();
    }
    bInventoryOpen = false;
    if (bPauseMenuOpen == false)
    {
        SwitchToGameInput();
    }
}

void APlayerCharacter::OpenEnhance()
{
    if (bEnhanceOpen) return;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC == nullptr) return;

    if (EnhanceScreenWidget == nullptr && EnhanceScreenWidgetClass)
    {
        EnhanceScreenWidget = CreateWidget<UUserWidget>(PC, EnhanceScreenWidgetClass);
    }
    if (EnhanceScreenWidget)
    {
        EnhanceScreenWidget->AddToViewport();
        if (UButton* CloseB = Cast<UButton>(EnhanceScreenWidget->GetWidgetFromName(TEXT("CloseBtn"))))
            CloseB->OnClicked.AddUniqueDynamic(this, &APlayerCharacter::CloseEnhance);
        bEnhanceOpen = true;
        SwitchToUIInput();
    }
}

void APlayerCharacter::CloseEnhance()
{
    if (bEnhanceOpen == false) return;
    if (EnhanceScreenWidget)
    {
        EnhanceScreenWidget->RemoveFromParent();
    }
    bEnhanceOpen = false;
    if (bInventoryOpen == false && bPauseMenuOpen == false && bShopOpen == false)
    {
        SwitchToGameInput();
    }
}

void APlayerCharacter::OpenEnhanceScreen()
{
    OpenEnhance();
}

void APlayerCharacter::OpenShopScreen(int32 ShopID)
{
    OpenShop(ShopID);
}

void APlayerCharacter::OpenShop(int32 ShopID)
{
    if (bShopOpen) return;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC == nullptr) return;

    if (ShopScreenWidget == nullptr && ShopScreenWidgetClass)
    {
        ShopScreenWidget = CreateWidget<UUserWidget>(PC, ShopScreenWidgetClass);
    }
    if (ShopScreenWidget)
    {
        ShopScreenWidget->AddToViewport();
        if (UShopScreenWidget* Shop = Cast<UShopScreenWidget>(ShopScreenWidget))
        {
            Shop->SetShopID(ShopID);
        }
        if (UButton* CloseB = Cast<UButton>(ShopScreenWidget->GetWidgetFromName(TEXT("CloseBtn"))))
            CloseB->OnClicked.AddUniqueDynamic(this, &APlayerCharacter::CloseShop);
        bShopOpen = true;
        SwitchToUIInput();
    }
}

void APlayerCharacter::CloseShop()
{
    if (bShopOpen == false) return;
    if (ShopScreenWidget)
    {
        ShopScreenWidget->RemoveFromParent();
    }
    bShopOpen = false;
    if (bInventoryOpen == false && bPauseMenuOpen == false && bEnhanceOpen == false)
    {
        SwitchToGameInput();
    }
}

void APlayerCharacter::OpenPauseMenu()
{
    if (bPauseMenuOpen) return;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC == nullptr) return;

    if (PauseMenuWidget == nullptr && PauseMenuWidgetClass)
    {
        PauseMenuWidget = CreateWidget<UUserWidget>(PC, PauseMenuWidgetClass);
    }
    if (PauseMenuWidget)
    {
        PauseMenuWidget->AddToViewport(10);
        bPauseMenuOpen = true;
        SwitchToUIInput();
        UGameplayStatics::SetGamePaused(this, true);
    }
}

void APlayerCharacter::ClosePauseMenu()
{
    if (bPauseMenuOpen == false) return;
    if (PauseMenuWidget)
    {
        PauseMenuWidget->RemoveFromParent();
    }
    bPauseMenuOpen = false;
    UGameplayStatics::SetGamePaused(this, false);
    if (bInventoryOpen == false)
    {
        SwitchToGameInput();
    }
}

void APlayerCharacter::SwitchToUIInput()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    PC->SetShowMouseCursor(true);
    PC->SetInputMode(FInputModeGameAndUI());

    // DefaultMappingContext는 유지 — I/F 등 토글 키가 UI 열린 상태에서도 살아있어야
    // 닫고 다시 열 수 있음 (제거하면 닫은 뒤 키가 죽어 재오픈 불가)
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
    {
        if (UIMappingContext)
        {
            Subsystem->AddMappingContext(UIMappingContext, 1);
        }
    }
}

void APlayerCharacter::SwitchToGameInput()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    PC->SetShowMouseCursor(false);
    PC->SetInputMode(FInputModeGameOnly());

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
    {
        if (UIMappingContext)
        {
            Subsystem->RemoveMappingContext(UIMappingContext);
        }
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void APlayerCharacter::StartDialogue(int32 InDialogueID, AActor* Interactor)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC == nullptr) return;

    if (DialogueWidget == nullptr && DialogueWidgetClass)
    {
        DialogueWidget = CreateWidget<UDialogueWidget>(PC, DialogueWidgetClass);
    }
    if (DialogueWidget == nullptr) return;

    if (DialogueWidget->IsInViewport() == false)
    {
        DialogueWidget->AddToViewport(5);
    }
    DialogueWidget->StartDialogue(InDialogueID, Interactor);
    SwitchToUIInput();
}

void APlayerCharacter::DebugDropItem(int32 ItemID, int32 Quantity)
{
    UGameInstance* GI = GetGameInstance();
    if (GI == nullptr)
    {
        return;
    }

    UItemSubsystem* ItemSub = GI->GetSubsystem<UItemSubsystem>();
    if (ItemSub == nullptr)
    {
        return;
    }

    if (HasAuthority())
    {
        // ItemID <= 0 → 인벤토리에 전체 지급, 그 외 → 바닥에 드롭
        if (ItemID <= 0)
        {
            DebugGiveAllItems(this);
            return;
        }
        const FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 150.f;
        ItemSub->SpawnItemInWorld(ItemID, Quantity, SpawnLoc, GetWorld());
        return;
    }

    Server_DebugDropItem(ItemID, Quantity);
}

void APlayerCharacter::Server_DebugDropItem_Implementation(int32 ItemID, int32 Quantity)
{
    UGameInstance* GI = GetGameInstance();
    if (GI == nullptr)
    {
        return;
    }

    UItemSubsystem* ItemSub = GI->GetSubsystem<UItemSubsystem>();
    if (ItemSub == nullptr)
    {
        return;
    }

    if (ItemID <= 0)
    {
        DebugGiveAllItems(this);
        return;
    }
    const FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 150.f;
    ItemSub->SpawnItemInWorld(ItemID, Quantity, SpawnLoc, GetWorld());
}
