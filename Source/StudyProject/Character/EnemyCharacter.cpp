#include "EnemyCharacter.h"
#include "GAS/CombatAbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "GAS/GA_EnemyAttack.h"
#include "Combat/EnemyCombatController.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
    AbilitySystemComponent = CreateDefaultSubobject<UCombatAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

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
