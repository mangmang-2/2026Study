#include "EnemyCharacter.h"
#include "Character/CharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "GAS/CombatAbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "GAS/GA_EnemyAttack.h"
#include "GAS/StudyGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Combat/EnemyCombatController.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

AEnemyCharacter::AEnemyCharacter()
{
    AbilitySystemComponent = CreateDefaultSubobject<UCombatAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    // AI라 소유 클라 연결이 없음 → Mixed 아닌 Minimal(태그/큐만 리플리케이트)
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    AttributeSet = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("AttributeSet"));

    // 전투 AI 컨트롤러 자동 빙의(배치/스폰 모두)
    AIControllerClass = AEnemyCombatController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // AI가 컨트롤러에서 직접 회전을 잡으므로 이동방향 자동회전 끔
    bUseControllerRotationYaw = false;
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->bOrientRotationToMovement = false;
    }

    AttackAbilityClass = UGA_EnemyAttack::StaticClass();

    // 카메라가 적 몸에 붙어 당겨지지 않게 Camera 채널 무시
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);

    // 오른손 무기 — HandSocket_R 소켓에 부착(소켓 트랜스폼이 그립을 결정)
    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EnemyWeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), TEXT("HandSocket_R"));
    WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
    WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponFinder(
        TEXT("/Game/SKnight_modular/Skeleton_Knight_07/mesh/weapon/SK_weapon.SK_weapon"));
    if (WeaponFinder.Succeeded())
    {
        WeaponMeshAsset = WeaponFinder.Object;
        WeaponMesh->SetSkeletalMeshAsset(WeaponMeshAsset);
    }

    // 왼손 방패 — HandSocket_L 소켓에 부착
    ShieldMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EnemyShieldMesh"));
    ShieldMesh->SetupAttachment(GetMesh(), TEXT("HandSocket_L"));
    ShieldMesh->SetCollisionProfileName(TEXT("NoCollision"));
    ShieldMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> ShieldFinder(
        TEXT("/Game/SKnight_modular/Skeleton_Knight_01/mesh/weapon/SK_shield_01.SK_shield_01"));
    if (ShieldFinder.Succeeded())
    {
        ShieldMeshAsset = ShieldFinder.Object;
        ShieldMesh->SetSkeletalMeshAsset(ShieldMeshAsset);
    }
}

UAbilitySystemComponent* AEnemyCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    InitAbilitySystem();
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
    // 스탠드얼론/클라: 아바타 정보 초기화 (서버는 PossessedBy에서도 처리)
    InitAbilitySystem();

    // 상태이상 둔화/스턴 반영용 — 평상시 기준 속도 캡처 + 태그 변화 구독
    if (GetCharacterMovement() != nullptr)
    {
        BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
        // MoveToActor를 가속도 기반으로 — 아니면 Acceleration=0이라 ABP가 idle로 미끄러짐
        if (FNavMovementProperties* NavProps = GetCharacterMovement()->GetNavMovementProperties())
        {
            NavProps->bUseAccelerationForPaths = true;
        }
    }

    // 워닝 데칼 등이 캐릭터 메시에 묻지 않도록(데칼은 바닥에만)
    if (GetMesh() != nullptr)
    {
        GetMesh()->SetReceivesDecals(false);
    }
    if (WeaponMesh != nullptr)
    {
        WeaponMesh->SetReceivesDecals(false);
    }
    if (ShieldMesh != nullptr)
    {
        ShieldMesh->SetReceivesDecals(false);
    }
    if (AbilitySystemComponent != nullptr)
    {
        AbilitySystemComponent->RegisterGameplayTagEvent(StudyTags::Status_Chilled, EGameplayTagEventType::NewOrRemoved)
            .AddUObject(this, &AEnemyCharacter::OnMoveStatusTagChanged);
        AbilitySystemComponent->RegisterGameplayTagEvent(StudyTags::Status_Shocked, EGameplayTagEventType::NewOrRemoved)
            .AddUObject(this, &AEnemyCharacter::OnMoveStatusTagChanged);
    }
}

void AEnemyCharacter::Multicast_StartGrowingDecal_Implementation(
    UMaterialInterface* DecalMaterial, FVector Anchor, FVector Dir, float FullLength, float Width, float Depth, float GrowTime)
{
    if (DecalMaterial == nullptr)
    {
        return;
    }
    // 기존 데칼 정리
    if (WarningDecalComp != nullptr)
    {
        WarningDecalComp->DestroyComponent();
        WarningDecalComp = nullptr;
    }

    DecalAnchor = Anchor;
    DecalDir = Dir.GetSafeNormal2D();
    DecalFullLength = FullLength;
    DecalHalfWidth = Width * 0.5f;
    DecalDepth = Depth;
    DecalGrowTime = FMath::Max(0.05f, GrowTime);
    DecalGrowElapsed = 0.f;

    // 보스 발밑에서 시작, Pitch -90 바닥 투영(로컬 Z = 돌진 방향)
    const FRotator DecalRot(-90.f, DecalDir.Rotation().Yaw, 0.f);
    const FVector StartSize(DecalDepth, DecalHalfWidth, 1.f);
    WarningDecalComp = UGameplayStatics::SpawnDecalAtLocation(this, DecalMaterial, StartSize, DecalAnchor, DecalRot, 0.f);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(DecalGrowTimer, this, &AEnemyCharacter::UpdateGrowingDecal, 0.03f, true);
    }
}

void AEnemyCharacter::UpdateGrowingDecal()
{
    if (WarningDecalComp == nullptr)
    {
        return;
    }
    DecalGrowElapsed += 0.03f;
    const float Alpha = FMath::Clamp(DecalGrowElapsed / DecalGrowTime, 0.f, 1.f);
    const float CurLen = DecalFullLength * Alpha;

    // near는 보스 고정, far가 성장 → 중심 = Anchor + Dir*CurLen/2
    const FVector Mid = DecalAnchor + DecalDir * (CurLen * 0.5f);
    WarningDecalComp->SetWorldLocation(FVector(Mid.X, Mid.Y, DecalAnchor.Z));
    WarningDecalComp->DecalSize = FVector(DecalDepth, DecalHalfWidth, FMath::Max(1.f, CurLen * 0.5f));
    WarningDecalComp->MarkRenderStateDirty();

    if (Alpha >= 1.f)
    {
        // 다 자라면 성장 정지(돌진 시작 시 Multicast_DestroyWarningDecal로 제거됨)
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(DecalGrowTimer);
        }
    }
}

void AEnemyCharacter::Multicast_DestroyWarningDecal_Implementation()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DecalGrowTimer);
    }
    if (WarningDecalComp != nullptr)
    {
        WarningDecalComp->DestroyComponent();
        WarningDecalComp = nullptr;
    }
}

void AEnemyCharacter::Multicast_EnterRagdoll_Implementation()
{
    // 래그돌 물리 전환은 플레이어와 동일 로직(ACharacterBase 정적 헬퍼) 공유
    ACharacterBase::ApplyRagdollPhysics(this);
}

void AEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AEnemyCharacter, bBlockStance);
}

void AEnemyCharacter::OnMoveStatusTagChanged(const FGameplayTag /*Tag*/, int32 /*NewCount*/)
{
    RefreshMoveSpeed();
}

void AEnemyCharacter::SetBaseWalkSpeed(float NewBaseSpeed)
{
    BaseWalkSpeed = NewBaseSpeed;
    RefreshMoveSpeed();
}

void AEnemyCharacter::RefreshMoveSpeed()
{
    UCharacterMovementComponent* Move = GetCharacterMovement();
    if (Move == nullptr || AbilitySystemComponent == nullptr)
    {
        return;
    }

    float Speed = BaseWalkSpeed;
    if (AbilitySystemComponent->HasMatchingGameplayTag(StudyTags::Status_Shocked))
    {
        Speed = 0.f;   // 스턴: 완전 정지
    }
    else if (AbilitySystemComponent->HasMatchingGameplayTag(StudyTags::Status_Chilled))
    {
        Speed = BaseWalkSpeed * ChillSpeedFactor;   // 둔화: 배율 적용
    }
    Move->MaxWalkSpeed = Speed;
}

void AEnemyCharacter::InitAbilitySystem()
{
    if (AbilitySystemComponent == nullptr)
    {
        return;
    }

    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    if (HasAuthority() == false || bAbilitiesGranted)
    {
        return;
    }

    // 스탯 초기화 GE
    if (DefaultAttributeEffect)
    {
        FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
        Ctx.AddSourceObject(this);
        FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributeEffect, 1.f, Ctx);
        if (Spec.IsValid())
        {
            AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    }

    // 적 전용 체력 세팅(플레이어와 공유하는 AttributeSet 기본값 100 대신 이 값으로)
    if (StartingMaxHP > 0.f)
    {
        AbilitySystemComponent->SetNumericAttributeBase(UCombatAttributeSet::GetMaxHPAttribute(), StartingMaxHP);
        AbilitySystemComponent->SetNumericAttributeBase(UCombatAttributeSet::GetHPAttribute(), StartingMaxHP);
    }

    // 반응 어빌리티 부여
    for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
    {
        if (AbilityClass)
        {
            AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
        }
    }

    // AI 공격 어빌리티 부여(AI 컨트롤러가 TryActivateAbilityByClass로 발동)
    if (AttackAbilityClass)
    {
        AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackAbilityClass, 1, INDEX_NONE, this));
    }

    bAbilitiesGranted = true;
}
