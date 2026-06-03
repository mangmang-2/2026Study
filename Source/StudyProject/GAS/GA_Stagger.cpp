#include "GA_Stagger.h"
#include "StudyGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_Stagger::UGA_Stagger()
{
    // 적(서버 권위) 반응이므로 서버에서만 실행
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // Event.Staggered 수신 시 자동 활성화
    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = StudyTags::Event_Staggered;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_Stagger);
    SetAssetTags(Tags);
}

void UGA_Stagger::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false || StaggerMontage == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 경직 상태 부여(처형/추가타 판정용) — EndAbility에서 해제
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(StudyTags::Status_Staggered);
    }

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, StaggerMontage, 1.0f);
    if (MontageTask == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_Stagger::OnMontageFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_Stagger::OnMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Stagger::OnMontageFinished);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_Stagger::OnMontageFinished);
    MontageTask->ReadyForActivation();
}

void UGA_Stagger::OnMontageFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Stagger::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(StudyTags::Status_Staggered);
    }
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
