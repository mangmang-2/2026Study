#include "GA_Dodge.h"
#include "StudyGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UGA_Dodge::UGA_Dodge()
{
    InputTag = StudyTags::Input_Dodge;
    bLocksMovement = true;

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_Dodge);
    SetAssetTags(Tags);
}

void UGA_Dodge::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false || DodgeMontages.Num() == 0)
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

    // 이동 입력 방향(월드) — 입력 없으면 백스텝
    FVector InputDir = FVector::ZeroVector;
    if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
    {
        InputDir = Move->GetLastInputVector();
    }
    InputDir.Z = 0.f;
    if (InputDir.SizeSquared() < KINDA_SMALL_NUMBER)
    {
        InputDir = -Char->GetActorForwardVector();
        InputDir.Z = 0.f;
    }
    InputDir = InputDir.GetSafeNormal();

    // 캐릭터 정면 기준 부호 각도(+우, -좌) → 8방향 인덱스
    const FVector Fwd = Char->GetActorForwardVector().GetSafeNormal2D();
    const float Cross = FVector::CrossProduct(Fwd, InputDir).Z;
    const float Dot = FVector::DotProduct(Fwd, InputDir);
    const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Cross, Dot));

    int32 Index = FMath::RoundToInt(AngleDeg / 45.f);   // -4..4
    if (Index < 0)
    {
        Index += 8;   // 0=F,1=FR,2=R,3=BR,4=B,5=BL,6=L,7=FL
    }
    Index = FMath::Clamp(Index, 0, DodgeMontages.Num() - 1);

    UAnimMontage* Montage = DodgeMontages[Index];
    if (Montage == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 회피 이동 — 고정 거리를 ease-out 보간으로 이동(지상/공중 동일, 끝에서 딱 정지).
    // 이동 시간을 회피 몽타주 길이에 맞춰 애니와 같이 끝나게(렉 느낌 방지).
    DodgeStartLoc = Char->GetActorLocation();
    DodgeEndLoc   = DodgeStartLoc + InputDir * DodgeDistance;
    DodgeElapsed  = 0.f;
    const float MontageLen = Montage->GetPlayLength();
    CurrentMoveDuration = (MontageLen > 0.f) ? (MontageLen * DodgeMoveFraction) : 0.18f;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(DodgeMoveTimer, this, &UGA_Dodge::DodgeMoveTick, 0.016f, true);
    }

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, 1.0f);
    if (MontageTask == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_Dodge::OnMontageFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_Dodge::OnMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Dodge::OnMontageFinished);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_Dodge::OnMontageFinished);
    MontageTask->ReadyForActivation();
}

void UGA_Dodge::OnMontageFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Dodge::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    StopDodgeMove();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Dodge::DodgeMoveTick()
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char == nullptr || CurrentMoveDuration <= 0.f)
    {
        StopDodgeMove();
        return;
    }

    DodgeElapsed += 0.016f;
    const float Alpha = FMath::Clamp(DodgeElapsed / CurrentMoveDuration, 0.f, 1.f);
    // 강한 ease-out(quartic): 시작에 확 치고 나가고 끝에서 빠르게 정지 → 미끄러짐(스케이트) 최소화
    const float Smooth = 1.f - FMath::Pow(1.f - Alpha, 4.f);

    FVector NewLoc = FMath::Lerp(DodgeStartLoc, DodgeEndLoc, Smooth);
    NewLoc.Z = Char->GetActorLocation().Z;   // 수직은 그대로(중력/지면 유지)

    Char->SetActorLocation(NewLoc, true);    // sweep: 벽 충돌 시 멈춤

    if (Alpha >= 1.f)
    {
        StopDodgeMove();
    }
}

void UGA_Dodge::StopDodgeMove()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DodgeMoveTimer);
    }
}
