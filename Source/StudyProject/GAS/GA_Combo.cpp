#include "GA_Combo.h"
#include "StudyGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/PlayerCharacter.h"
#include "Character/EnemyCharacter.h"
#include "Combat/LockOnComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

const TArray<TObjectPtr<UAnimMontage>>& UGA_Combo::SelectCombo(const FWeaponComboData& Data) const
{
    return Data.GroundCombo;
}

UGA_Combo::UGA_Combo()
{
    InputTag = StudyTags::Input_Attack;
    bLocksMovement = true;

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_Combo);
    SetAssetTags(Tags);
}

void UGA_Combo::ActivateAbility(
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

    // 장착 무기로 콤보 데이터 조회
    FWeaponComboData ComboData;
    if (UComboLibrary::GetWeaponComboData(GetAvatarActorFromActorInfo(), ComboDataTable, DefaultComboRow, ComboData) == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CurrentMontages = SelectCombo(ComboData);
    if (CurrentMontages.Num() == 0)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CurrentDamage = ComboData.DamagePerHit;
    CurrentHitFeel = ComboData.HitFeel;
    CurrentPlayRate = (ComboData.AttackPlayRate > 0.f) ? ComboData.AttackPlayRate : 1.0f;

    // 타격 판정 시점은 몽타주의 Melee Hit 노티파이가 결정
    MeleeDamage = ComboData.DamagePerHit;
    MeleeHitFeel = ComboData.HitFeel;
    MeleeHitEventTag = HitEventTag;
    MeleeHitEventMagnitude = HitEventMagnitude;
    StartMeleeHitWindowListeners();

    CurrentStepIn = ComboData.StepIn;

    // 새 콤보 시작 — 1타부터
    ComboIndex = 0;
    bInputBuffered = false;
    PlayComboMontage();
}

void UGA_Combo::NotifyComboInput()
{
    // 다음 타가 있을 때만 예약
    if (ComboIndex + 1 >= CurrentMontages.Num())
    {
        return;
    }

    bInputBuffered = true;

    // 이미 캔슬 윈도우가 열려 있으면 회복 동작을 캔슬하고 즉시 다음 타로
    if (bWindowOpen)
    {
        AdvanceCombo();
    }
}

void UGA_Combo::PlayComboMontage()
{
    if (CurrentMontages.IsValidIndex(ComboIndex) == false)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    UAnimMontage* Montage = CurrentMontages[ComboIndex];
    if (Montage == nullptr)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    // 새 타 시작 — 윈도우/체인 상태 리셋
    bWindowOpen = false;
    bChained = false;

    // 마지막 타는 LastHit(슬램), 아니면 일반
    const bool bLastHit = (ComboIndex == CurrentMontages.Num() - 1);

    // 상태이상은 마지막 타에만(옵션 끄면 모든 타)
    bApplyStatusThisSwing = bStatusOnLastHitOnly ? bLastHit : true;

    if (bLastHit && LastHitEventTag.IsValid())
    {
        MeleeHitEventTag = LastHitEventTag;
        MeleeHitEventMagnitude = LastHitEventMagnitude;
    }
    else
    {
        MeleeHitEventTag = HitEventTag;
        MeleeHitEventMagnitude = HitEventMagnitude;
    }

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, Montage, CurrentPlayRate, NAME_None, false);
    if (MontageTask == nullptr)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    // 재생 속도만큼 히트·윈도우 타이밍도 보정
    const float RateScale = (CurrentPlayRate > 0.f) ? (1.f / CurrentPlayRate) : 1.f;

    // 블렌드아웃은 안전망(윈도우 캔슬 없을 때만 체이닝/종료), 외부 인터럽트는 콤보 종료
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_Combo::OnComboBlendOut);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Combo::OnComboInterrupted);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_Combo::OnComboInterrupted);

    // 몽타주 교체 시 이전 태스크의 OnInterrupted 무시 가드(ReadyForActivation 동안만)
    bAdvancing = true;
    MontageTask->ReadyForActivation();
    bAdvancing = false;

    // 전진키+앞쪽 타겟이면 조금 접근
    TryStepInToTarget();

    // 캔슬 윈도우 하한용 기준 시간(히트 판정 자체는 노티파이가 결정)
    const float ScaledHitDelay = HitDelay * RateScale;

    // 캔슬 윈도우 오픈 시점(히트 판정보다 뒤)
    const float Length = Montage->GetPlayLength() * RateScale;
    float WindowTime = Length * ComboWindowFraction;
    if (WindowTime < ScaledHitDelay + 0.02f)
    {
        WindowTime = ScaledHitDelay + 0.02f;
    }

    UAbilityTask_WaitDelay* WindowTask = UAbilityTask_WaitDelay::WaitDelay(this, WindowTime);
    if (WindowTask != nullptr)
    {
        WindowTask->OnFinish.AddDynamic(this, &UGA_Combo::OnComboWindowOpen);
        WindowTask->ReadyForActivation();
    }
}

void UGA_Combo::OnMeleeHitLanded()
{
    // 공중 콤보면 타격 프레임에 자신도 체공
    if (bFloatSelfOnHit)
    {
        ApplySelfFloat();
    }
}

void UGA_Combo::ApplySelfFloat()
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

    if (bSelfFloatActive == false)
    {
        // 다른 시스템이 이미 낮췄으면 1.0을 복원 기준으로
        const float Cur = Move->GravityScale;
        SavedSelfGravity = (Cur >= 0.9f) ? Cur : 1.0f;
        bSelfFloatActive = true;
        Char->LandedDelegate.AddDynamic(this, &UGA_Combo::OnSelfLanded);
    }
    Move->GravityScale = SelfHangGravityScale;

    // 위로 안 띄우고 낙하만 멈춰 제자리 체공(매 타격 솟던 문제 수정)
    Move->Velocity.Z = SelfPopZ;
}

void UGA_Combo::OnSelfLanded(const FHitResult& /*Hit*/)
{
    RestoreSelfGravity();
}

void UGA_Combo::RestoreSelfGravity()
{
    if (bSelfFloatActive == false)
    {
        return;
    }
    if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        Char->LandedDelegate.RemoveDynamic(this, &UGA_Combo::OnSelfLanded);
        if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
        {
            Move->GravityScale = SavedSelfGravity;
        }
    }
    bSelfFloatActive = false;
}

void UGA_Combo::OnComboWindowOpen()
{
    // 버퍼된 입력이 있으면 즉시 다음 타로
    bWindowOpen = true;
    if (bInputBuffered)
    {
        AdvanceCombo();
    }
}

void UGA_Combo::AdvanceCombo()
{
    // 이미 넘어갔으면 무시
    if (bChained)
    {
        return;
    }
    if (bInputBuffered == false || ComboIndex + 1 >= CurrentMontages.Num())
    {
        return;
    }

    bChained = true;
    bInputBuffered = false;
    ComboIndex++;
    PlayComboMontage();
}

void UGA_Combo::OnComboBlendOut()
{
    // 이미 윈도우에서 캔슬해 다음 타로 넘어갔으면 무시
    if (bChained)
    {
        return;
    }

    // 막판 버퍼 입력은 여기서 체이닝(안전망)
    if (bInputBuffered && ComboIndex + 1 < CurrentMontages.Num())
    {
        AdvanceCombo();
        return;
    }

    // 더 이을 입력이 없으면 콤보 종료
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Combo::OnComboInterrupted()
{
    // 의도적 몽타주 교체로 생긴 인터럽트는 무시
    if (bAdvancing)
    {
        return;
    }

    // 피격 등 외부 요인으로 끊기면 콤보 종료
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Combo::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    StopStepIn();
    StopMeleeHitWindow();
    RestoreSelfGravity();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ── 스텝인 ──────────────────────────────────────────────────────────────────

AActor* UGA_Combo::FindStepInTarget(const FVector& SelfLoc, const FVector& Forward) const
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }

    AActor* Best = nullptr;
    float BestDistSq = CurrentStepIn.MaxRange * CurrentStepIn.MaxRange;

    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacter::StaticClass(), Enemies);
    for (AActor* E : Enemies)
    {
        if (E == nullptr)
        {
            continue;
        }
        const FVector To = E->GetActorLocation() - SelfLoc;
        const FVector ToFlat(To.X, To.Y, 0.f);
        const float DistSq = ToFlat.SizeSquared();
        if (DistSq > BestDistSq || DistSq < KINDA_SMALL_NUMBER)
        {
            continue;
        }
        // 정면(앞쪽) 적만
        if (FVector::DotProduct(ToFlat.GetSafeNormal(), Forward) < 0.f)
        {
            continue;
        }
        BestDistSq = DistSq;
        Best = E;
    }
    return Best;
}

void UGA_Combo::TryStepInToTarget()
{
    if (CurrentStepIn.bEnable == false)
    {
        return;
    }

    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char == nullptr)
    {
        return;
    }

    // 이동 잠금 중에도 입력값은 기록됨
    AActor* TargetActor = nullptr;
    if (APlayerCharacter* Player = Cast<APlayerCharacter>(Char))
    {
        if (Player->GetMoveInput().Y < StepInForwardThreshold)
        {
            return;   // 전진키를 누르고 있지 않음
        }
        // 록온 중이면 그 타겟 우선
        if (ULockOnComponent* Lock = Player->GetLockOnComponent())
        {
            if (Lock->IsLockedOn())
            {
                TargetActor = Lock->GetCurrentTarget();
            }
        }
    }
    else
    {
        return;   // 플레이어가 아니면 스텝인 안 함
    }

    const FVector SelfLoc = Char->GetActorLocation();
    const FVector Forward = Char->GetActorForwardVector().GetSafeNormal2D();

    if (TargetActor == nullptr)
    {
        TargetActor = FindStepInTarget(SelfLoc, Forward);
    }
    if (TargetActor == nullptr)
    {
        return;
    }

    // 타겟 방향(수평)
    FVector ToTarget = TargetActor->GetActorLocation() - SelfLoc;
    ToTarget.Z = 0.f;
    const float Dist = ToTarget.Size();
    const FVector Dir = (Dist > KINDA_SMALL_NUMBER) ? (ToTarget / Dist) : Forward;

    // 목표 회전(Yaw만) — 이동 없어도 회전은 함
    StepInTargetRot = FRotator(0.f, Dir.Rotation().Yaw, 0.f);

    // 정지 거리 밖이면 접근(최대 MaxStep), 안이면 회전만
    const float MoveDist = (Dist > CurrentStepIn.StopDistance)
        ? FMath::Min(Dist - CurrentStepIn.StopDistance, CurrentStepIn.MaxStep)
        : 0.f;

    StepInStartLoc = SelfLoc;
    StepInEndLoc = SelfLoc + Dir * MoveDist;
    StepInElapsed = 0.f;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            StepInTimerHandle, this, &UGA_Combo::StepInTick, 0.016f, true);
    }
    // 첫 틱을 기다리지 않고 즉시 이동(모션과 동시 전진)
    StepInTick();
}

void UGA_Combo::StepInTick()
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char == nullptr || CurrentStepIn.Duration <= 0.f)
    {
        StopStepIn();
        return;
    }

    StepInElapsed += 0.016f;
    const float Alpha = FMath::Clamp(StepInElapsed / CurrentStepIn.Duration, 0.f, 1.f);
    // cubic ease-out(시작 강하게, 끝 감속)
    const float Smooth = 1.f - FMath::Pow(1.f - Alpha, 3.f);

    FVector NewLoc = FMath::Lerp(StepInStartLoc, StepInEndLoc, Smooth);
    NewLoc.Z = Char->GetActorLocation().Z;   // 수직은 그대로(지면 따라감)

    Char->SetActorLocation(NewLoc, true);    // sweep=true: 벽/적 충돌 존중

    // Yaw만 부드럽게 회전
    if (CurrentStepIn.RotateSpeed > 0.f)
    {
        const FRotator NewRot = FMath::RInterpTo(Char->GetActorRotation(), StepInTargetRot, 0.016f, CurrentStepIn.RotateSpeed);
        Char->SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
    }

    if (Alpha >= 1.f)
    {
        StopStepIn();
    }
}

void UGA_Combo::StopStepIn()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(StepInTimerHandle);
    }
}
