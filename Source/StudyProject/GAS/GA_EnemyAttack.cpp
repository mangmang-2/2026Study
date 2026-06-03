#include "GA_EnemyAttack.h"
#include "StudyGameplayTags.h"
#include "GE_Damage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

UGA_EnemyAttack::UGA_EnemyAttack()
{
    // AI(서버)에서만 실행
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    bLocksMovement = true;   // 공격 중 이동 정지

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_EnemyAttack);
    SetAssetTags(Tags);

    // 데미지 GE(C++ UGE_Damage). BP 없이 C++로 부여되므로 여기서 지정해야 데미지 적용됨.
    DamageGEClass = UGE_Damage::StaticClass();

    // 기본 공격 몽타주(없으면 BP에서 지정). 콤보 몽타주를 재사용(Melee Hit 노티파이 포함)
    static ConstructorHelpers::FObjectFinder<UAnimMontage> MontageFinder(
        TEXT("/Game/GAS/Abilities/Montages/AM_Combo_01.AM_Combo_01"));
    if (MontageFinder.Succeeded())
    {
        AttackMontage = MontageFinder.Object;
    }
}

void UGA_EnemyAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false || AttackMontage == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 타격 판정 파라미터 + 노티파이 윈도우 리스너(몽타주 Melee Hit 노티파이가 판정 시점 결정).
    // 이벤트 태그는 비움 → GE_Damage 적용 시 AttributeSet가 알아서 Event.HitReact를 보냄.
    MeleeDamage = AttackDamage;
    MeleeHitFeel = AttackHitFeel;
    MeleeHitEventTag = FGameplayTag();
    MeleeHitEventMagnitude = 0.f;
    StartMeleeHitWindowListeners();

    const float Rate = (AttackPlayRate > 0.f) ? AttackPlayRate : 1.0f;
    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage, Rate);
    if (MontageTask == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyAttack::OnAttackFinished);
    MontageTask->ReadyForActivation();
}

void UGA_EnemyAttack::OnAttackFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyAttack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    StopMeleeHitWindow();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
