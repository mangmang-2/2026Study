#include "GA_HitReact.h"
#include "StudyGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_HitReact::UGA_HitReact()
{
    // 적 반응은 서버 권위
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // 연속 피격마다 플린치 재생(재생 중이어도 재발동)
    bRetriggerInstancedAbility = true;
    bLocksMovement = true;

    // 슈퍼아머 중엔 플린치 차단(데미지는 들어감)
    ActivationBlockedTags.AddTag(StudyTags::Status_SuperArmor);

    // Event.HitReact 수신 시 자동 활성화
    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = StudyTags::Event_HitReact;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::State_HitReact);
    SetAssetTags(Tags);
}

void UGA_HitReact::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false || HitMontage == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 피격 시 진행 중인 공격 어빌 취소 — 적 공격(GA_EnemyAttack)은 ChainTimer로 콤보를 잇기에
    // 취소 안 하면 첫 타를 맞고도 다음 타이머가 공격 몽타주를 덮어써 "바로 공격"한다.
    // 플레이어 콤보도 피격 시 끊기게 함.
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        FGameplayTagContainer CancelTags;
        CancelTags.AddTag(StudyTags::Ability_EnemyAttack);
        CancelTags.AddTag(StudyTags::Ability_Combo);
        CancelTags.AddTag(StudyTags::Ability_AirCombo);
        ASC->CancelAbilities(&CancelTags, nullptr, this);
    }

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, HitMontage, 1.0f);
    if (MontageTask == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_HitReact::OnMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_HitReact::OnMontageFinished);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_HitReact::OnMontageFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_HitReact::OnMontageFinished);
    MontageTask->ReadyForActivation();
}

void UGA_HitReact::OnMontageFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
