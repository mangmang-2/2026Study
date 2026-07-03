#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Inventory/TradeComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "GAS/CombatAbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "GAS/StudyGameplayTags.h"
#include "TimerManager.h"
#include "GAS/GE_StatusBurning.h"
#include "GAS/GE_StatusBleeding.h"
#include "GAS/GE_StatusShocked.h"
#include "GAS/GE_StatusChilled.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "Combat/LockOnComponent.h"
#include "Skills/SkillManagerComponent.h"
#include "UI/HUD/SkillHUDWidget.h"
#include "UI/HUD/SkillCastBarWidget.h"
#include "UI/HUD/SkillTreeWidget.h"
#include "UI/HUD/HUDWidget.h"
#include "UI/Dialogue/DialogueWidget.h"
#include "UI/Shop/ShopScreenWidget.h"
#include "UI/Menu/GameMenuShellWidget.h"
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
#include "GameFramework/Character.h"

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

    // 범위 내 IInteractable 감지
    InteractionDetector = CreateDefaultSubobject<UInteractionDetectorComponent>(TEXT("InteractionDetector"));
    InteractionDetector->SetupAttachment(RootComponent);

    // 록온 컴포넌트
    LockOnComp = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComp"));

    // 스킬 매니저(슬롯/쿨다운/발동)
    SkillManagerComp = CreateDefaultSubobject<USkillManagerComponent>(TEXT("SkillManagerComp"));

    // 강화 화면 단축키 기본값(에디터에서 재지정)
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

    // 사망/리스폰: 초기 스폰 위치 저장 + HP 0(State.Dead) 감지
    SpawnTransform = GetActorTransform();
    if (UAbilitySystemComponent* DeathASC = GetAbilitySystemComponent())
    {
        DeathASC->RegisterGameplayTagEvent(StudyTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
            .AddUObject(this, &APlayerCharacter::OnDeathTagChanged);
    }

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

    // SP/HP 바 위젯(WBP_SPBar) — 실제 HP/SP를 갱신하는 체력바. 스킬 HUD와 좌우 폭(X28,360) 정렬됨.
    if (IsLocallyControlled() && SPBarWidgetClass)
    {
        SPBarWidget = CreateWidget<UUserWidget>(PC, SPBarWidgetClass);
        if (SPBarWidget)
        {
            SPBarWidget->AddToViewport(1);
        }
    }

    // 스킬 HUD(하단 중앙 슬롯) + 시전 바 — 코드 전용 위젯, 로컬 플레이어만
    if (IsLocallyControlled() && SkillManagerComp != nullptr)
    {
        SkillHUDWidget = CreateWidget<USkillHUDWidget>(PC, USkillHUDWidget::StaticClass());
        if (SkillHUDWidget != nullptr)
        {
            SkillHUDWidget->InitHUD(SkillManagerComp);
            SkillHUDWidget->AddToViewport(2);
        }

        SkillCastBarWidget = CreateWidget<USkillCastBarWidget>(PC, USkillCastBarWidget::StaticClass());
        if (SkillCastBarWidget != nullptr)
        {
            SkillCastBarWidget->InitCastBar(SkillManagerComp);
            SkillCastBarWidget->AddToViewport(2);
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

    InitialMeshTransform = GetMesh()->GetRelativeTransform();
    InitialMeshProfile = GetMesh()->GetCollisionProfileName();
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

    // ── 장비 세트(로드아웃) ───────────────────────────────────────────
    if (LoadoutSlot1Action)
    {
        EIC->BindAction(LoadoutSlot1Action, ETriggerEvent::Started, this, &APlayerCharacter::HandleLoadoutSlot1);
    }
    if (LoadoutSlot2Action)
    {
        EIC->BindAction(LoadoutSlot2Action, ETriggerEvent::Started, this, &APlayerCharacter::HandleLoadoutSlot2);
    }
    if (LoadoutSlot3Action)
    {
        EIC->BindAction(LoadoutSlot3Action, ETriggerEvent::Started, this, &APlayerCharacter::HandleLoadoutSlot3);
    }

    // 숫자키 1/2/3 → 장비 세트 전환(IMC/IA 설정 없이 직접 바인딩)
    PlayerInputComponent->BindKey(EKeys::One,   IE_Pressed, this, &APlayerCharacter::HandleLoadoutSlot1);
    PlayerInputComponent->BindKey(EKeys::Two,   IE_Pressed, this, &APlayerCharacter::HandleLoadoutSlot2);
    PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &APlayerCharacter::HandleLoadoutSlot3);

    // Z/X/C → 스킬 슬롯 0/1/2 (IMC/IA 설정 없이 직접 바인딩. Q=록온·R=처형과 충돌 피함)
    // Pressed=조준 시작(PointTarget) 또는 즉발, Released=조준 발동
    PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed,  this, &APlayerCharacter::HandleSkill1);
    PlayerInputComponent->BindKey(EKeys::X, IE_Pressed,  this, &APlayerCharacter::HandleSkill2);
    PlayerInputComponent->BindKey(EKeys::C, IE_Pressed,  this, &APlayerCharacter::HandleSkill3);
    PlayerInputComponent->BindKey(EKeys::Z, IE_Released, this, &APlayerCharacter::HandleSkillReleased1);
    PlayerInputComponent->BindKey(EKeys::X, IE_Released, this, &APlayerCharacter::HandleSkillReleased2);
    PlayerInputComponent->BindKey(EKeys::C, IE_Released, this, &APlayerCharacter::HandleSkillReleased3);

    // T → 스킬트리(슬롯 배정) 토글
    PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &APlayerCharacter::ToggleSkillTree);
}

// ── 입력 핸들러 ──────────────────────────────────────────────────────────────

void APlayerCharacter::HandleMove(const FInputActionValue& Value)
{
    // 이동이 잠겨도 입력값 자체는 기록(콤보 스텝인의 전진키 눌림 판정용)
    LastMoveInput = Value.Get<FVector2D>();

    // 공격/회피/처형 등 모션 중에는 실제 이동은 무시
    if (UCombatAbilitySystemComponent* ASC = Cast<UCombatAbilitySystemComponent>(GetAbilitySystemComponent()))
    {
        if (ASC->IsMovementLocked() || ASC->HasMatchingGameplayTag(StudyTags::Status_Shocked))
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
    // 2단계 GAS UseItem GA로 대체 예정(현재 빈 훅)
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
        // 위젯이 중앙 정렬이라 투영 지점에 그대로 배치
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

// ── 장비 세트(로드아웃) ───────────────────────────────────────────────────────

void APlayerCharacter::HandleLoadoutSlot1()
{
    ApplyLoadoutSlot(0);
}

void APlayerCharacter::HandleLoadoutSlot2()
{
    ApplyLoadoutSlot(1);
}

void APlayerCharacter::HandleLoadoutSlot3()
{
    ApplyLoadoutSlot(2);
}

void APlayerCharacter::ApplyLoadoutSlot(int32 Index)
{
    if (UEquipmentComponent* Equip = GetEquipmentComponent())
    {
        Equip->ApplyLoadout(Index);
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
    // 공중에선 런처 금지(연속 우클릭으로 계속 떠오르는 문제 방지) — 지상에서만 발동
    if (GetCharacterMovement() != nullptr && GetCharacterMovement()->IsFalling())
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

void APlayerCharacter::UseSkill(int32 SlotIndex)
{
    if (SkillManagerComp != nullptr)
    {
        SkillManagerComp->ActivateSlot(SlotIndex);
    }
}

void APlayerCharacter::HandleSkill1()
{
    UseSkill(0);
}

void APlayerCharacter::HandleSkill2()
{
    UseSkill(1);
}

void APlayerCharacter::HandleSkill3()
{
    UseSkill(2);
}

void APlayerCharacter::HandleSkillReleased1()
{
    if (SkillManagerComp != nullptr)
    {
        SkillManagerComp->ReleaseSlot(0);
    }
}

void APlayerCharacter::HandleSkillReleased2()
{
    if (SkillManagerComp != nullptr)
    {
        SkillManagerComp->ReleaseSlot(1);
    }
}

void APlayerCharacter::HandleSkillReleased3()
{
    if (SkillManagerComp != nullptr)
    {
        SkillManagerComp->ReleaseSlot(2);
    }
}

void APlayerCharacter::ToggleSkillTree()
{
    if (IsLocallyControlled() == false || SkillManagerComp == nullptr)
    {
        return;
    }

    if (bSkillTreeOpen)
    {
        if (SkillTreeWidget != nullptr)
        {
            SkillTreeWidget->RemoveFromParent();
        }
        bSkillTreeOpen = false;
        SwitchToGameInput();
        return;
    }

    if (SkillTreeWidget == nullptr)
    {
        SkillTreeWidget = CreateWidget<USkillTreeWidget>(Cast<APlayerController>(GetController()), USkillTreeWidget::StaticClass());
    }
    if (SkillTreeWidget != nullptr)
    {
        // AddToViewport가 RebuildWidget(위젯 트리 생성)을 먼저 돌려야 InitTree의
        // RefreshList가 ListBox에 행을 채울 수 있다(순서 중요).
        SkillTreeWidget->AddToViewport(20);
        SkillTreeWidget->InitTree(SkillManagerComp);
        bSkillTreeOpen = true;
        SwitchToUIInput();
    }
}

// ── 상태이상 디버그(콘솔) ────────────────────────────────────────────────────
// 록온 타깃(없으면 가장 가까운 적)에게 상태이상 GE를 부여. 서버 권위에서 적용.

void APlayerCharacter::DebugBurn()
{
    ApplyDebugStatus(UGE_StatusBurning::StaticClass());
}

void APlayerCharacter::DebugBleed()
{
    ApplyDebugStatus(UGE_StatusBleeding::StaticClass());
}

void APlayerCharacter::DebugShock()
{
    ApplyDebugStatus(UGE_StatusShocked::StaticClass());
}

void APlayerCharacter::DebugChill()
{
    ApplyDebugStatus(UGE_StatusChilled::StaticClass());
}

AEnemyCharacter* APlayerCharacter::FindStatusTarget() const
{
    // 1순위: 록온 타깃이 적이면 그 대상
    if (LockOnComp != nullptr)
    {
        if (AEnemyCharacter* Locked = Cast<AEnemyCharacter>(LockOnComp->GetCurrentTarget()))
        {
            return Locked;
        }
    }

    // 2순위: 정면 우선 없이 단순 최근접 적(사거리 1200 내)
    AEnemyCharacter* Best = nullptr;
    float BestDistSq = 1200.f * 1200.f;
    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacter::StaticClass(), Found);
    for (AActor* Actor : Found)
    {
        AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Actor);
        if (Enemy == nullptr)
        {
            continue;
        }
        const float DistSq = FVector::DistSquared(Enemy->GetActorLocation(), GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Enemy;
        }
    }
    return Best;
}

void APlayerCharacter::ApplyDebugStatus(TSubclassOf<UGameplayEffect> StatusGE)
{
    if (StatusGE == nullptr)
    {
        return;
    }

    // 상태이상 적용은 서버 권위 — 클라면 서버로 위임
    if (HasAuthority() == false)
    {
        Server_ApplyDebugStatus(StatusGE);
        return;
    }

    AEnemyCharacter* Target = FindStatusTarget();
    if (Target == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Status] 대상 적이 없습니다(록온 또는 1200 내 적 필요)."));
        return;
    }

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    if (TargetASC == nullptr)
    {
        return;
    }

    FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
    Ctx.AddInstigator(this, this);
    FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(StatusGE, 1.f, Ctx);
    if (Spec.IsValid())
    {
        TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}

void APlayerCharacter::Server_ApplyDebugStatus_Implementation(TSubclassOf<UGameplayEffect> StatusGE)
{
    ApplyDebugStatus(StatusGE);
}

// ── 사망 / 리스폰 ────────────────────────────────────────────────────────────

void APlayerCharacter::OnDeathTagChanged(const FGameplayTag /*Tag*/, int32 NewCount)
{
    if (NewCount > 0)
    {
        HandleDeath();
    }
}

void APlayerCharacter::HandleDeath()
{
    if (bIsDead)
    {
        return;
    }
    bIsDead = true;

    // 입력/이동 정지와 래그돌은 GA_Death가 처리. 여기선 리스폰 타이머만.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(RespawnTimer, this, &APlayerCharacter::Respawn, FMath::Max(0.1f, RespawnDelay), false);
    }
}

void APlayerCharacter::Respawn()
{
    // 사망 태그 해제 + HP/SP 회복
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        ASC->RemoveLooseGameplayTag(StudyTags::State_Dead);
    }
    if (UCombatAttributeSet* Attr = GetCombatAttributeSet())
    {
        Attr->SetHP(Attr->GetMaxHP());
        Attr->SetSP(Attr->GetMaxSP());
    }

    // GA_Death 취소(누운 상태 유지 중인 어빌리티 종료) — 래그돌 해제보다 먼저
    FGameplayTagContainer DeathTag;
    DeathTag.AddTag(StudyTags::State_Dead);
    GetAbilitySystemComponent()->CancelAbilities(&DeathTag);

    // 래그돌 해제(메시 재부착·물리 정지·캡슐/이동/스무딩 복원)를 전 클라에 적용 —
    // 안 그러면 다른 클라엔 시체만 남고 리스폰이 안 보인다.
    Multicast_ExitRagdoll();

    // 초기 스폰 위치로 복귀(서버 → 이동 복제로 클라 반영)
    SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

    // 입력 복원
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        EnableInput(PC);
    }

    bIsDead = false;
}

void APlayerCharacter::HandleSprintStart()
{
    bIsSprinting = true;
    // 기준속도로 설정 — 둔화/스턴 상태이상이 합성 반영됨
    SetBaseWalkSpeed(600.f);
}

void APlayerCharacter::HandleSprintEnd()
{
    bIsSprinting = false;
    SetBaseWalkSpeed(300.f);
}

// ── UI 열기/닫기 ─────────────────────────────────────────────────────────────

bool APlayerCharacter::IsAnyScreenOpen() const
{
    return bInventoryOpen || bEnhanceOpen || bShopOpen || bTradeOpen || bPauseMenuOpen;
}

void APlayerCharacter::CloseExclusiveScreens()
{
    // 전체화면 패널은 동시에 하나만 — 새로 열기 전에 나머지 닫음(거래는 네트워크 흐름이라 제외)
    if (bInventoryOpen)
    {
        CloseInventory();
    }
    if (bEnhanceOpen)
    {
        CloseEnhance();
    }
    if (bShopOpen)
    {
        CloseShop();
    }
    if (bPauseMenuOpen)
    {
        ClosePauseMenu();
    }
}

// 인벤/장비/강화는 모두 통합 탭 셸(UGameMenuShellWidget)을 사용. 탭만 다르게 연다.
void APlayerCharacter::OpenMenu(int32 Tab)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC == nullptr) return;

    // 셸 외 전체화면 패널(상점/일시정지)은 닫음. 거래는 네트워크 흐름이라 제외.
    if (bShopOpen)      CloseShop();
    if (bPauseMenuOpen) ClosePauseMenu();

    if (MenuShell == nullptr)
    {
        MenuShell = CreateWidget<UGameMenuShellWidget>(PC, UGameMenuShellWidget::StaticClass());
    }
    if (MenuShell == nullptr) return;

    if (MenuShell->IsInViewport() == false)
    {
        MenuShell->AddToViewport(7);
    }
    MenuShell->OnCloseRequested.AddUniqueDynamic(this, &APlayerCharacter::CloseMenu);
    MenuShell->InitShell(Tab);   // 콘텐츠 생성·바인딩 + 탭 선택 (AddToViewport 이후라 위젯트리 준비됨)

    bInventoryOpen = true;       // 셸 열림 마스터 플래그
    SwitchToUIInput();
    MenuShell->SetKeyboardFocus();
}

void APlayerCharacter::CloseMenu()
{
    if (bInventoryOpen == false) return;

    if (HasAuthority())
    {
        SaveCharacter();
    }
    if (MenuShell)
    {
        MenuShell->RemoveFromParent();
    }
    bInventoryOpen = false;
    if (IsAnyScreenOpen() == false)
    {
        SwitchToGameInput();
    }
}

void APlayerCharacter::OpenInventory() { OpenMenu(UGameMenuShellWidget::TAB_Inventory); }
void APlayerCharacter::CloseInventory() { CloseMenu(); }
void APlayerCharacter::OpenEnhance() { OpenMenu(UGameMenuShellWidget::TAB_Enhance); }
void APlayerCharacter::CloseEnhance() { CloseMenu(); }

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

    CloseExclusiveScreens();

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
    if (IsAnyScreenOpen() == false)
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

    CloseExclusiveScreens();

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
    if (IsAnyScreenOpen() == false)
    {
        SwitchToGameInput();
    }
}

void APlayerCharacter::HandleTradeUpdated()
{
    if (TradeComp == nullptr) return;

    // 거래 시작이면 화면 열기, 종료면 닫기(원격 클라는 PartnerActor OnRep로 진입)
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

void APlayerCharacter::DebugAddGold(int32 Amount)
{
    if (HasAuthority())
    {
        AddGold(Amount);
    }
    else
    {
        Server_DebugAddGold(Amount);
    }
}

void APlayerCharacter::Server_DebugAddGold_Implementation(int32 Amount)
{
    AddGold(Amount);
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

void APlayerCharacter::SpawnTestDummy()
{
    if (!HasAuthority()) { Server_SpawnTestDummy(); return; }
    if (TestEnemyClass == nullptr || GetWorld() == nullptr) { return; }
    const FVector Loc = GetActorLocation() + GetActorForwardVector() * 450.f + FVector(0.f, 0.f, 60.f);
    const FRotator Rot = GetActorRotation() + FRotator(0.f, 180.f, 0.f);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AActor* Spawned = GetWorld()->SpawnActor<AActor>(TestEnemyClass, Loc, Rot, Params);

    // AI 끄기 — 컨트롤러 제거(자동 언포제스). ASC/HP는 EnemyCharacter::BeginPlay서 init돼 데미지는 받음.
    if (ACharacter* C = Cast<ACharacter>(Spawned))
    {
        if (AController* Ctrl = C->GetController())
        {
            Ctrl->Destroy();
        }
        if (C->GetCharacterMovement() != nullptr)
        {
            C->GetCharacterMovement()->StopMovementImmediately();
        }
    }
}
void APlayerCharacter::Server_SpawnTestDummy_Implementation() { SpawnTestDummy(); }

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

    CloseExclusiveScreens();

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
    if (IsAnyScreenOpen() == false)
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

    SetGameplayHudVisible(false);   // 전체화면 UI 진입 → 게임플레이 HUD 숨김

    // DefaultMappingContext 유지 — 제거하면 UI 닫은 뒤 토글 키가 죽어 재오픈 불가
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

    SetGameplayHudVisible(true);   // 게임플레이 복귀 → HUD 복원

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

void APlayerCharacter::SetGameplayHudVisible(bool bVisible)
{
    // 패시브 HUD(체력바/스킬/캐스트바)는 입력을 막지 않게 SelfHitTestInvisible로 복원.
    const ESlateVisibility VPassive     = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
    // 적 소환 위젯은 버튼 클릭이 필요하므로 Visible로 복원.
    const ESlateVisibility VInteractive = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

    if (HUDWidget)          HUDWidget->SetVisibility(VPassive);
    if (SPBarWidget)        SPBarWidget->SetVisibility(VPassive);
    if (SkillHUDWidget)     SkillHUDWidget->SetVisibility(VPassive);
    if (SkillCastBarWidget) SkillCastBarWidget->SetVisibility(VPassive);
    if (SpawnerWidget)      SpawnerWidget->SetVisibility(VInteractive);
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
