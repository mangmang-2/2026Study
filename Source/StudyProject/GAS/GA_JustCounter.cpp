#include "GA_JustCounter.h"
#include "StudyGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UGA_JustCounter::UGA_JustCounter()
{
    InputTag = StudyTags::Input_Parry;
    bLocksMovement = true;

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_JustCounter);
    SetAssetTags(Tags);
}

void UGA_JustCounter::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    bCountering = false;

    // 패리 윈도우 오픈 — Status.Parrying 부여, ParryWindow 후 자동 닫힘
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(StudyTags::Status_Parrying);
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ParryWindowTimer, this, &UGA_JustCounter::CloseParryWindow, ParryWindow, false);
    }

    // 패리 성공 이벤트 대기(ApplyMeleeDamage가 보냄)
    if (UAbilityTask_WaitGameplayEvent* ParryTask =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, StudyTags::Event_Parried, nullptr, /*OnlyTriggerOnce=*/true))
    {
        ParryTask->EventReceived.AddDynamic(this, &UGA_JustCounter::OnParrySuccess);
        ParryTask->ReadyForActivation();
    }

    // 패리 자세 몽타주(짧은 가드). 없어도 윈도우는 동작.
    if (ParryStanceMontage != nullptr)
    {
        if (UAbilityTask_PlayMontageAndWait* StanceTask =
            UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ParryStanceMontage, 1.0f))
        {
            StanceTask->OnCompleted.AddDynamic(this, &UGA_JustCounter::OnStanceFinished);
            StanceTask->OnBlendOut.AddDynamic(this, &UGA_JustCounter::OnStanceFinished);
            StanceTask->OnInterrupted.AddDynamic(this, &UGA_JustCounter::OnStanceFinished);
            StanceTask->OnCancelled.AddDynamic(this, &UGA_JustCounter::OnStanceFinished);
            StanceTask->ReadyForActivation();
        }
    }
}

void UGA_JustCounter::OnParrySuccess(FGameplayEventData /*Payload*/)
{
    // 이미 카운터 중이면 무시
    if (bCountering)
    {
        return;
    }
    bCountering = true;

    // 윈도우 즉시 닫기(중복 패리 방지)
    CloseParryWindow();

    // 리포스트 데미지 판정 파라미터 + 노티파이 윈도우 리스너(CounterMontage의 Melee Hit 노티파이)
    MeleeDamage = CounterDamage;
    MeleeHitFeel = CounterHitFeel;
    MeleeHitEventTag = FGameplayTag();
    MeleeHitEventMagnitude = 0.f;
    StartMeleeHitWindowListeners();

    if (CounterMontage == nullptr)
    {
        // 카운터 몽타주 없으면 바로 종료
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    if (UAbilityTask_PlayMontageAndWait* CounterTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CounterMontage, 1.0f))
    {
        CounterTask->OnCompleted.AddDynamic(this, &UGA_JustCounter::OnCounterFinished);
        CounterTask->OnBlendOut.AddDynamic(this, &UGA_JustCounter::OnCounterFinished);
        CounterTask->OnInterrupted.AddDynamic(this, &UGA_JustCounter::OnCounterFinished);
        CounterTask->OnCancelled.AddDynamic(this, &UGA_JustCounter::OnCounterFinished);
        CounterTask->ReadyForActivation();
    }
    else
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void UGA_JustCounter::OnStanceFinished()
{
    // 카운터로 넘어갔으면(스탠스 몽타주가 카운터에 의해 끊긴 것) 무시
    if (bCountering)
    {
        return;
    }
    // 패리 못 하고 자세만 끝남 — 종료
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_JustCounter::OnCounterFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_JustCounter::CloseParryWindow()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ParryWindowTimer);
    }
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(StudyTags::Status_Parrying);
    }
}

void UGA_JustCounter::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    CloseParryWindow();
    StopMeleeHitWindow();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
