#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Inventory/TradeComponent.h"
#include "Inventory/InventoryComponent.h"
#include "UI/HUD/HUDWidget.h"
#include "UI/Dialogue/DialogueWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Interaction/InteractionDetectorComponent.h"
#include "Interaction/InteractionPromptComponent.h"
#include "Interaction/InteractableInterface.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"

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
    // 이전 포커스 프롬프트 숨기기
    if (LastFocusedActor)
    {
        if (UInteractionPromptComponent* Prompt = LastFocusedActor->FindComponentByClass<UInteractionPromptComponent>())
            Prompt->HidePrompt();
    }

    LastFocusedActor = NewFocus;

    if (!NewFocus) return;

    UInteractionPromptComponent* Prompt = NewFocus->FindComponentByClass<UInteractionPromptComponent>();
    UE_LOG(LogTemp, Warning, TEXT("[Prompt] OnFocusChanged: %s | PromptComp=%s"),
        *NewFocus->GetName(), Prompt ? TEXT("FOUND") : TEXT("NULL"));
    if (Prompt)
    {
        if (NewFocus->Implements<UInteractable>())
        {
            FText PromptText = IInteractable::Execute_GetInteractionPrompt(NewFocus);
            UE_LOG(LogTemp, Warning, TEXT("[Prompt] ShowPrompt: %s"), *PromptText.ToString());
            Prompt->ShowPrompt(PromptText);
        }
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

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->RemoveMappingContext(DefaultMappingContext);
        }
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

    const FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 150.f;

    if (HasAuthority())
    {
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

    const FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 150.f;
    ItemSub->SpawnItemInWorld(ItemID, Quantity, SpawnLoc, GetWorld());
}
