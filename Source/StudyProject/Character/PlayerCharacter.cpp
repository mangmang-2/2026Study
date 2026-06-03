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
#include "Inventory/EquipmentComponent.h"
#include "GAS/CombatAbilitySystemComponent.h"
#include "GAS/StudyGameplayTags.h"
#include "Combat/LockOnComponent.h"
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
#include "Engine/World.h"
#include "Character/EnemyCharacter.h"
#include "UI/HUD/EnemySpawnerWidget.h"

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

        // 디버그: 강화/상점/거래 테스트용 골드 지급
        PC->AddGold(100000);

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

    // 록온 컴포넌트
    LockOnComp = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComp"));

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

    static ConstructorHelpers::FClassFinder<UUserWidget> TradeWBPFinder(
        TEXT("/Game/UI/Trade/WBP_TradeScreenWidget"));
    if (TradeWBPFinder.Succeeded()) TradeScreenWidgetClass = TradeWBPFinder.Class;

    static ConstructorHelpers::FClassFinder<UUserWidget> PromptWBPFinder(
        TEXT("/Game/UI/Common/WBP_InteractionPrompt"));
    if (PromptWBPFinder.Succeeded()) InteractionPromptClass = PromptWBPFinder.Class;

    static ConstructorHelpers::FClassFinder<UUserWidget> SPBarWBPFinder(
        TEXT("/Game/UI/HUD/WBP_SPBar"));
    if (SPBarWBPFinder.Succeeded()) SPBarWidgetClass = SPBarWBPFinder.Class;

    // 디버그 스폰 기본값
    static ConstructorHelpers::FClassFinder<AActor> EnemyBPFinder(TEXT("/Game/Blueprints/BP_Enemy"));
    if (EnemyBPFinder.Succeeded()) TestEnemyClass = EnemyBPFinder.Class;
    static ConstructorHelpers::FClassFinder<AActor> BossBPFinder(TEXT("/Game/Blueprints/BP_Boss"));
    if (BossBPFinder.Succeeded()) BossSpawnClass = BossBPFinder.Class;
    SpawnerWidgetClass = UEnemySpawnerWidget::StaticClass();

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

    // SP/HP 바 위젯
    if (IsLocallyControlled() && SPBarWidgetClass)
    {
        SPBarWidget = CreateWidget<UUserWidget>(PC, SPBarWidgetClass);
        if (SPBarWidget)
        {
            SPBarWidget->AddToViewport(1);
        }
    }

    // 디버그 적/보스 스폰 위젯(우상단). 콘솔 ToggleSpawner로 표시/숨김
    if (IsLocallyControlled() && SpawnerWidgetClass)
    {
        SpawnerWidget = CreateWidget<UUserWidget>(PC, SpawnerWidgetClass);
        if (SpawnerWidget)
        {
            SpawnerWidget->AddToViewport(3);
        }
    }

    // 포커스 변경 이벤트 — 상호작용 프롬프트 표시/숨기기
    if (IsLocallyControlled())
    {
        InteractionDetector->OnFocusChanged.AddDynamic(this, &APlayerCharacter::OnFocusChanged);

        // 거래 상태 변화 → 거래 화면 자동 열기/닫기
        if (TradeComp)
        {
            TradeComp->OnTradeUpdated.AddDynamic(this, &APlayerCharacter::HandleTradeUpdated);
            TradeComp->OnTradeResult.AddDynamic(this, &APlayerCharacter::HandleTradeResult);
        }
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
        EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &APlayerCharacter::HandleMoveCompleted);
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

    // ── GAS 어빌리티 입력 ───────────────────────────────────────────
    if (AttackAction)
    {
        EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleAttack);
    }
    if (HeavyAttackAction)
    {
        EIC->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleLauncher);
    }
    if (FinisherAction)
    {
        EIC->BindAction(FinisherAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleFinisher);
    }
    if (LockOnAction)
    {
        EIC->BindAction(LockOnAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleLockOn);
    }
    if (LockOnSwitchAction)
    {
        EIC->BindAction(LockOnSwitchAction, ETriggerEvent::Triggered, this, &APlayerCharacter::HandleLockOnSwitch);
    }
    if (DodgeAction)
    {
        EIC->BindAction(DodgeAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleDodge);
    }
    if (ParryAction)
    {
        EIC->BindAction(ParryAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleParry);
    }
}

// ── 입력 핸들러 ──────────────────────────────────────────────────────────────

void APlayerCharacter::HandleMove(const FInputActionValue& Value)
{
    // 이동이 잠겨도 입력값 자체는 기록(콤보 스텝인의 전진키 눌림 판정용)
    LastMoveInput = Value.Get<FVector2D>();

    // 공격/회피/처형 등 모션 중에는 실제 이동은 무시
    if (UCombatAbilitySystemComponent* ASC = Cast<UCombatAbilitySystemComponent>(GetAbilitySystemComponent()))
    {
        if (ASC->IsMovementLocked())
        {
            return;
        }
    }

    const FVector2D MoveVec = Value.Get<FVector2D>();
    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MoveVec.Y);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MoveVec.X);
    }
}

void APlayerCharacter::HandleMoveCompleted(const FInputActionValue& /*Value*/)
{
    // 키에서 손 떼면 입력값 초기화(Triggered가 더 이상 안 들어오므로 직접 클리어)
    LastMoveInput = FVector2D::ZeroVector;
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
    if (InteractionPromptW == nullptr) return;

    // 대화/강화/상점/인벤/일시정지 등 UI가 떠 있으면 상호작용 프롬프트(F:강화 등)는 숨김
    const bool bDialogueOpen = (DialogueWidget != nullptr && DialogueWidget->IsInViewport());
    const bool bUIOpen = bDialogueOpen || bInventoryOpen || bPauseMenuOpen || bEnhanceOpen || bShopOpen;
    if (bUIOpen || LastFocusedActor == nullptr)
    {
        if (InteractionPromptW->GetVisibility() != ESlateVisibility::Collapsed)
        {
            InteractionPromptW->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    // 포커스 유지 중 대화가 닫혔다면 다시 표시(복구)
    if (InteractionPromptW->GetVisibility() == ESlateVisibility::Collapsed)
    {
        InteractionPromptW->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

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

// ── GAS 어빌리티 입력 ───────────────────────────────────────────────────────

void APlayerCharacter::HandleAttack()
{
    UCombatAbilitySystemComponent* ASC = Cast<UCombatAbilitySystemComponent>(GetAbilitySystemComponent());
    if (ASC == nullptr)
    {
        return;
    }
    // 낙하/점프 중이면 공중 콤보, 아니면 지상 콤보로 라우팅
    const bool bFalling = (GetCharacterMovement() != nullptr) && GetCharacterMovement()->IsFalling();
    ASC->TryActivateAbilityByInputTag(bFalling ? StudyTags::Input_AirAttack : StudyTags::Input_Attack);
}

void APlayerCharacter::HandleLauncher()
{
    UCombatAbilitySystemComponent* ASC = Cast<UCombatAbilitySystemComponent>(GetAbilitySystemComponent());
    if (ASC == nullptr)
    {
        return;
    }
    ASC->TryActivateAbilityByInputTag(StudyTags::Input_Launcher);
}

void APlayerCharacter::HandleFinisher()
{
    UCombatAbilitySystemComponent* ASC = Cast<UCombatAbilitySystemComponent>(GetAbilitySystemComponent());
    if (ASC == nullptr)
    {
        return;
    }
    ASC->TryActivateAbilityByInputTag(StudyTags::Input_Finisher);
}

void APlayerCharacter::HandleLockOn()
{
    if (LockOnComp != nullptr)
    {
        LockOnComp->ToggleLockOn();
    }
}

void APlayerCharacter::HandleLockOnSwitch(const FInputActionValue& Value)
{
    if (LockOnComp == nullptr)
    {
        return;
    }
    const float Dir = Value.Get<float>();
    if (FMath::Abs(Dir) > 0.1f)
    {
        LockOnComp->SwitchTarget(Dir);
    }
}

void APlayerCharacter::HandleDodge()
{
    UCombatAbilitySystemComponent* ASC = Cast<UCombatAbilitySystemComponent>(GetAbilitySystemComponent());
    if (ASC == nullptr)
    {
        return;
    }
    ASC->TryActivateAbilityByInputTag(StudyTags::Input_Dodge);
}

void APlayerCharacter::HandleParry()
{
    UCombatAbilitySystemComponent* ASC = Cast<UCombatAbilitySystemComponent>(GetAbilitySystemComponent());
    if (ASC == nullptr)
    {
        return;
    }
    ASC->TryActivateAbilityByInputTag(StudyTags::Input_Parry);
}

void APlayerCharacter::DebugParry()
{
    HandleParry();
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

// ── 거래 ─────────────────────────────────────────────────────────────
void APlayerCharacter::OpenTradeScreen()
{
    if (bTradeOpen) return;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC == nullptr) return;

    if (TradeScreenWidget == nullptr && TradeScreenWidgetClass)
    {
        TradeScreenWidget = CreateWidget<UUserWidget>(PC, TradeScreenWidgetClass);
    }
    if (TradeScreenWidget)
    {
        TradeScreenWidget->AddToViewport(8);
        bTradeOpen = true;
        SwitchToUIInput();
    }
}

void APlayerCharacter::CloseTradeScreen()
{
    if (TradeScreenWidget)
    {
        TradeScreenWidget->RemoveFromParent();
    }
    bTradeOpen = false;
    if (bInventoryOpen == false && bPauseMenuOpen == false && bEnhanceOpen == false && bShopOpen == false)
    {
        SwitchToGameInput();
    }
}

void APlayerCharacter::HandleTradeUpdated()
{
    if (TradeComp == nullptr) return;

    // 거래 시작(상대 지정) → 화면 열기, 거래 종료(상대 해제) → 화면 닫기
    // (원격 클라이언트는 PartnerActor 복제(OnRep)로 이 경로를 탐)
    if (TradeComp->IsTrading() && bTradeOpen == false)
    {
        OpenTradeScreen();
    }
    else if (TradeComp->IsTrading() == false && bTradeOpen)
    {
        CloseTradeScreen();
    }
}

void APlayerCharacter::HandleTradeResult(bool /*bSuccess*/)
{
    // 거래 성사/취소 모두 화면 닫음
    CloseTradeScreen();
}

void APlayerCharacter::DebugTradeNearest()
{
    if (TradeComp == nullptr) return;

    APawn* Self = this;
    APlayerCharacter* Nearest = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Players);
    for (AActor* A : Players)
    {
        if (A == Self) continue;
        const float D = FVector::DistSquared(A->GetActorLocation(), GetActorLocation());
        if (D < BestDistSq)
        {
            BestDistSq = D;
            Nearest = Cast<APlayerCharacter>(A);
        }
    }

    if (Nearest)
    {
        TradeComp->RequestTrade(Nearest);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Trade] 거래할 다른 플레이어가 없습니다(2인 플레이 필요)."));
    }
}

void APlayerCharacter::EnhanceWeapon()
{
    if (UEquipmentComponent* Equip = GetEquipmentComponent())
    {
        Equip->EnhanceEquippedWeapon(1);
    }
}

// ── 디버그 스폰 ──────────────────────────────────────────────────────────────

void APlayerCharacter::SpawnTestEnemy()
{
    if (!HasAuthority()) { Server_SpawnTestEnemy(); return; }
    if (TestEnemyClass == nullptr || GetWorld() == nullptr) { return; }
    const FVector Loc = GetActorLocation() + GetActorForwardVector() * 450.f + FVector(0.f, 0.f, 60.f);
    const FRotator Rot = GetActorRotation() + FRotator(0.f, 180.f, 0.f);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    GetWorld()->SpawnActor<AActor>(TestEnemyClass, Loc, Rot, Params);
}
void APlayerCharacter::Server_SpawnTestEnemy_Implementation() { SpawnTestEnemy(); }

void APlayerCharacter::SpawnTestBoss()
{
    if (!HasAuthority()) { Server_SpawnTestBoss(); return; }
    if (BossSpawnClass == nullptr || GetWorld() == nullptr) { return; }
    const FVector Loc = GetActorLocation() + GetActorForwardVector() * 600.f + FVector(0.f, 0.f, 90.f);
    const FRotator Rot = GetActorRotation() + FRotator(0.f, 180.f, 0.f);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    GetWorld()->SpawnActor<AActor>(BossSpawnClass, Loc, Rot, Params);
}
void APlayerCharacter::Server_SpawnTestBoss_Implementation() { SpawnTestBoss(); }

void APlayerCharacter::ClearAllEnemies()
{
    if (!HasAuthority()) { Server_ClearEnemies(); return; }
    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(this, AEnemyCharacter::StaticClass(), Enemies);
    for (AActor* E : Enemies)
    {
        if (E) { E->Destroy(); }
    }
}
void APlayerCharacter::Server_ClearEnemies_Implementation() { ClearAllEnemies(); }

void APlayerCharacter::ToggleSpawner()
{
    if (SpawnerWidget)
    {
        const bool bSpawnerHidden = (SpawnerWidget->GetVisibility() == ESlateVisibility::Collapsed);
        SpawnerWidget->SetVisibility(bSpawnerHidden ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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
