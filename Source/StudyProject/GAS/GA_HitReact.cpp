#include "GA_HitReact.h"
#include "StudyGameplayTags.h"
#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_HitReact::UGA_HitReact()
{
    // 적(서버 권위) 반응이므로 서버에서만 실행
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // 연속 피격마다 플린치가 다시 재생되도록(이미 재생 중이어도 재발동)
    bRetriggerInstancedAbility = true;

    // 피격 중에는 이동 불가(플린치 동안 굳음)
    bLocksMovement = true;

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
