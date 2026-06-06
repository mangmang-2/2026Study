#include "GA_AirLaunch.h"
#include "StudyGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UGA_AirLaunch::UGA_AirLaunch()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // 저글 시 떠 있어도 재발동
    bRetriggerInstancedAbility = true;

    // Launched(저글)/Slammed(내려찍기) 이벤트로 자동 활성화
    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = StudyTags::Event_Launched;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);

    FAbilityTriggerData SlamTrigger;
    SlamTrigger.TriggerTag = StudyTags::Event_Slammed;
    SlamTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(SlamTrigger);

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

    const bool bSlam = (TriggerEventData != nullptr) && (TriggerEventData->EventTag == StudyTags::Event_Slammed);
    const float Mag = (TriggerEventData != nullptr) ? TriggerEventData->EventMagnitude : 0.f;

    // 착지 콜백(착지 시 넉다운→기상)
    Char->LandedDelegate.AddDynamic(this, &UGA_AirLaunch::OnLanded);

    if (bSlam)
    {
        // ── 공중콤보 마무리: 바닥으로 내려찍기 ──────────────────────────
        const float Down = (Mag > 0.f) ? Mag : SlamDownSpeed;
        if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
        {
            if (bGravityActive == false)
            {
                SavedGravityScale = Move->GravityScale;
                bGravityActive = true;
            }
            Move->GravityScale = SlamGravityScale;   // 빠르게 내리꽂힘
        }

        // 공격자 반대로 밀어 사선 낙하
        FVector Horiz = FVector::ZeroVector;
        if (TriggerEventData != nullptr && TriggerEventData->Instigator != nullptr)
        {
            FVector Away = Char->GetActorLocation() - TriggerEventData->Instigator->GetActorLocation();
            Away.Z = 0.f;
            Away = Away.GetSafeNormal();
            if (Away.IsNearlyZero())
            {
                Away = Char->GetActorForwardVector().GetSafeNormal2D();
            }
            Horiz = Away * SlamHorizSpeed;
        }
        Char->LaunchCharacter(FVector(Horiz.X, Horiz.Y, -Down), true, true);

        UAnimMontage* SlamM = (SlamMontage != nullptr) ? SlamMontage : AirHitMontage;
        if (SlamM != nullptr)
        {
            if (UAbilityTask_PlayMontageAndWait* Task =
                UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SlamM, 1.0f))
            {
                Task->ReadyForActivation();
            }
        }
        return;   // 상승/체공 로직 건너뜀
    }

    // ── 일반 런치/저글: 위로 발사(상승은 정상 중력, 정점부터 체공 중력) ──
    float LaunchZ = (Mag > 0.f) ? Mag : DefaultLaunchZ;
    ApplyLaunchGravity();
    Char->LaunchCharacter(FVector(0.f, 0.f, LaunchZ), true, true);

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

    // 원래 중력으로 복원
    RestoreGravity();

    // 넉다운 표시(이 동안 타격 불가)
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(StudyTags::State_Knockdown);
    }

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
    // 몽타주 교체 시 넉다운 OnInterrupt가 재진입하므로 한 번만 기상하도록 가드
    if (bGetUpStarted)
    {
        return;
    }
    bGetUpStarted = true;

    // 기상 몽타주 → 끝나면 어빌 종료
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
    // 기상 완료 — 넉다운 해제
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(StudyTags::State_Knockdown);
    }
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

    // 이미 관리 중이면 원래 중력값 보존(체공 중력 저장 방지)
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

    // 정점(상승속도 ≤0) → 체공 중력 전환
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
    // 취소돼도 떠 있는 채 안 남게 정리
    if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        Char->LandedDelegate.RemoveDynamic(this, &UGA_AirLaunch::OnLanded);
    }
    RestoreGravity();
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(StudyTags::State_Knockdown);   // 취소돼도 넉다운 태그 잔류 방지
    }
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
