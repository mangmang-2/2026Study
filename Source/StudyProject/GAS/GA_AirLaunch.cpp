#include "GA_AirLaunch.h"
#include "StudyGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UGA_AirLaunch::UGA_AirLaunch()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // 공중 콤보가 Event.Launched를 다시 쏘면(저글) 이미 떠 있어도 재발동되게
    bRetriggerInstancedAbility = true;

    // Event.Launched 수신 시 자동 활성화
    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = StudyTags::Event_Launched;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::State_AirBorne);
    SetAssetTags(Tags);
}

void UGA_AirLaunch::ActivateAbility(
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

    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    bGetUpStarted = false;   // 이번 활성화의 기상 가드 초기화

    float LaunchZ = DefaultLaunchZ;
    if (TriggerEventData != nullptr && TriggerEventData->EventMagnitude > 0.f)
    {
        LaunchZ = TriggerEventData->EventMagnitude;
    }

    // 착지 콜백 등록 후 위로 발사 (상승은 정상 중력, 정점부터 체공 중력)
    Char->LandedDelegate.AddDynamic(this, &UGA_AirLaunch::OnLanded);
    ApplyLaunchGravity();
    Char->LaunchCharacter(FVector(0.f, 0.f, LaunchZ), true, true);

    // 공중 피격 몽타주(있으면)
    if (AirHitMontage != nullptr)
    {
        UAbilityTask_PlayMontageAndWait* MontageTask =
            UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AirHitMontage, 1.0f);
        if (MontageTask != nullptr)
        {
            MontageTask->ReadyForActivation();
        }
    }
}

void UGA_AirLaunch::OnLanded(const FHitResult& Hit)
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char != nullptr)
    {
        Char->LandedDelegate.RemoveDynamic(this, &UGA_AirLaunch::OnLanded);
    }

    // 땅에 닿았으니 원래 중력으로 복원
    RestoreGravity();

    if (KnockdownMontage == nullptr)
    {
        OnKnockdownFinished();
        return;
    }

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, KnockdownMontage, 1.0f);
    if (MontageTask == nullptr)
    {
        OnKnockdownFinished();
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_AirLaunch::OnKnockdownFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_AirLaunch::OnKnockdownFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_AirLaunch::OnKnockdownFinished);
    MontageTask->ReadyForActivation();
}

void UGA_AirLaunch::OnKnockdownFinished()
{
    // 넉다운 몽타주를 기상 몽타주로 교체할 때 넉다운 태스크의 OnInterrupted가 다시 들어오므로
    // 한 번만 기상으로 진행하도록 가드(중복 시작 시 첫 기상이 끊겨 안 보이는 버그 방지)
    if (bGetUpStarted)
    {
        return;
    }
    bGetUpStarted = true;

    // 넉다운(쓰러짐) 후 일어서는 몽타주 재생 → 끝나면 어빌 종료
    if (GetUpMontage == nullptr)
    {
        OnGetUpFinished();
        return;
    }

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, GetUpMontage, 1.0f);
    if (MontageTask == nullptr)
    {
        OnGetUpFinished();
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_AirLaunch::OnGetUpFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_AirLaunch::OnGetUpFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_AirLaunch::OnGetUpFinished);
    MontageTask->ReadyForActivation();
}

void UGA_AirLaunch::OnGetUpFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_AirLaunch::ApplyLaunchGravity()
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char == nullptr)
    {
        return;
    }
    UCharacterMovementComponent* Move = Char->GetCharacterMovement();
    if (Move == nullptr)
    {
        return;
    }

    // 이미 관리 중이면 원래 중력값을 덮어쓰지 않음(체공 중력 0.35가 저장되는 사고 방지)
    if (bGravityActive == false)
    {
        SavedGravityScale = Move->GravityScale;
        bGravityActive = true;
    }
    Move->GravityScale = LaunchGravityScale;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ApexTimerHandle, this, &UGA_AirLaunch::CheckApex, 0.016f, true);
    }
}

void UGA_AirLaunch::CheckApex()
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    UCharacterMovementComponent* Move = Char ? Char->GetCharacterMovement() : nullptr;
    if (Move == nullptr)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(ApexTimerHandle);
        }
        return;
    }

    // 정점 도달(상승 속도가 0 이하) → 체공 중력으로 전환 후 감시 종료
    if (Move->Velocity.Z <= 0.f)
    {
        Move->GravityScale = HangGravityScale;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(ApexTimerHandle);
        }
    }
}

void UGA_AirLaunch::RestoreGravity()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ApexTimerHandle);
    }
    if (bGravityActive)
    {
        if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
        {
            if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
            {
                Move->GravityScale = SavedGravityScale;
            }
        }
        bGravityActive = false;
    }
}

void UGA_AirLaunch::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 안전하게 착지 델리게이트 해제 + 중력 복원(취소돼도 떠 있는 채 안 남게)
    if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        Char->LandedDelegate.RemoveDynamic(this, &UGA_AirLaunch::OnLanded);
    }
    RestoreGravity();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
