#include "CombatGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "StudyGameplayTags.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "CollisionShape.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ComboData.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Character/CharacterBase.h"
#include "Character/EnemyCharacter.h"
#include "Camera/CameraShakeBase.h"
#include "TimerManager.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "DrawDebugHelpers.h"

// 공격범위 디버그 시각화 (study.DrawMeleeRange 1)
static TAutoConsoleVariable<int32> CVarDrawMeleeRange(
    TEXT("study.DrawMeleeRange"),
    0,
    TEXT("Draw melee attack range (sweep) debug shape. 0=off, 1=on"),
    ECVF_Default);

UCombatGameplayAbility::UCombatGameplayAbility()
{
    // 콤보/회피 등은 인스턴스별 상태가 필요
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 서버 권위 + 클라 예측
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UCombatGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);

    if (bActivateOnGranted && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
    }
}

bool UCombatGameplayAbility::ApplyMeleeDamage(float DamageAmount, FGameplayTag EventOnHit, float EventMagnitude, const FHitFeel& Feel,
    TSet<TWeakObjectPtr<AActor>>* AlreadyHit)
{
    if (DamageGEClass == nullptr)
    {
        return false;
    }

    AActor* Avatar = GetAvatarActorFromActorInfo();
    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
    if (Avatar == nullptr || SourceASC == nullptr)
    {
        return false;
    }
    UWorld* World = Avatar->GetWorld();
    if (World == nullptr)
    {
        return false;
    }

    // 타격 판정은 서버 권위. 예측 클라(LocalPredicted)에선 트레이스/넉백/이벤트 스킵 —
    // 시각효과는 서버가 Multicast_HitFeedback으로 전 클라에 뿌림.
    if (Avatar->HasAuthority() == false)
    {
        return false;
    }

    const FVector Start = Avatar->GetActorLocation();
    const FVector End = Start + Avatar->GetActorForwardVector() * MeleeRange;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Avatar);

    TArray<FHitResult> Hits;
    World->SweepMultiByChannel(
        Hits, Start, End, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeSphere(MeleeRadius), Params);

#if ENABLE_DRAW_DEBUG
    // 스피어 스윕을 캡슐로 시각화
    if (CVarDrawMeleeRange.GetValueOnGameThread() != 0)
    {
        const FVector Seg = End - Start;
        const float Dist = Seg.Size();
        const FVector Dir = Dist > KINDA_SMALL_NUMBER ? Seg / Dist : Avatar->GetActorForwardVector();
        const FVector Center = (Start + End) * 0.5f;
        const FQuat Rot = FQuat::FindBetweenNormals(FVector::UpVector, Dir);
        DrawDebugCapsule(World, Center, Dist * 0.5f + MeleeRadius, MeleeRadius, Rot,
            FColor::Yellow, /*persistent*/false, /*lifetime*/0.05f, /*depthprio*/0, /*thickness*/1.0f);
    }
#endif

    bool bHitAny = false;
    TSet<AActor*> Done;
    TArray<TWeakObjectPtr<AActor>> StoppedActors;   // 히트스톱 복원 대상

    // 공격자 진영(적=AEnemyCharacter, 그 외=플레이어)
    const bool bAvatarIsEnemy = Avatar->IsA(AEnemyCharacter::StaticClass());

    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (Target == nullptr || Done.Contains(Target))
        {
            continue;
        }
        Done.Add(Target);

        // 같은 진영은 데미지 안 줌(아군 오사 방지)
        if (Target->IsA(AEnemyCharacter::StaticClass()) == bAvatarIsEnemy)
        {
            continue;
        }

        // 한 스윙 내 같은 대상 중복 타격 방지
        if (AlreadyHit != nullptr && AlreadyHit->Contains(Target))
        {
            continue;
        }

        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
        if (TargetASC == nullptr)
        {
            continue;
        }

        // 죽었거나 넉다운된 대상은 제외
        if (TargetASC->HasMatchingGameplayTag(StudyTags::State_Dead)
            || TargetASC->HasMatchingGameplayTag(StudyTags::State_Knockdown))
        {
            continue;
        }

        // ── 저스트카운터(패리) 가로채기 ────────────────────────────────
        // 패리 윈도우 중 + 공격자 정면이면: 데미지 무효 + 공격자 경직 / 패리 측 리포스트
        if (TargetASC->HasMatchingGameplayTag(StudyTags::Status_Parrying))
        {
            FVector ToAttacker = Avatar->GetActorLocation() - Target->GetActorLocation();
            ToAttacker.Z = 0.f;
            const FVector TgtFwd = Target->GetActorForwardVector().GetSafeNormal2D();
            if (FVector::DotProduct(TgtFwd, ToAttacker.GetSafeNormal()) >= ParryFacingDot)
            {
                // 공격자(적) 경직
                {
                    FGameplayEventData StaggerP;
                    StaggerP.EventTag = StudyTags::Event_Staggered;
                    StaggerP.Instigator = Target;
                    StaggerP.Target = Avatar;
                    SourceASC->HandleGameplayEvent(StudyTags::Event_Staggered, &StaggerP);
                }
                // 패리 성공 — 리포스트 GA가 수신
                {
                    FGameplayEventData ParryP;
                    ParryP.EventTag = StudyTags::Event_Parried;
                    ParryP.Instigator = Avatar;
                    ParryP.Target = Target;
                    TargetASC->HandleGameplayEvent(StudyTags::Event_Parried, &ParryP);
                }
                // 이 스윙에서 재판정 방지
                if (AlreadyHit != nullptr)
                {
                    AlreadyHit->Add(Target);
                }
                continue;
            }
        }

        // 타격 피드백(VFX+플래시+데미지넘버) — 서버 멀티캐스트로 전 클라 표시
        const FVector FxLoc = Hit.ImpactPoint.IsZero() ? Target->GetActorLocation() : FVector(Hit.ImpactPoint);
        ACharacterBase* AvatarChar = Cast<ACharacterBase>(Avatar);
        if (AvatarChar != nullptr && Avatar->HasAuthority())
        {
            AvatarChar->Multicast_HitFeedback(Feel.HitEffect, FxLoc, Hit.ImpactNormal, Target, FMath::RoundToInt(DamageAmount), false);
        }
        else if (AvatarChar == nullptr && Feel.HitEffect != nullptr)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Feel.HitEffect, FxLoc, FRotator(Hit.ImpactNormal.Rotation()));
        }

        // 런치 등 이벤트 먼저(피격 반응보다 우선)
        if (EventOnHit.IsValid())
        {
            FGameplayEventData Payload;
            Payload.EventTag = EventOnHit;
            Payload.Instigator = Avatar;
            Payload.Target = Target;
            Payload.EventMagnitude = EventMagnitude;
            TargetASC->HandleGameplayEvent(EventOnHit, &Payload);
        }

        // 데미지 GE 적용 (SetByCaller Data.Damage)
        FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
        Ctx.AddSourceObject(Avatar);
        FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageGEClass, 1.f, Ctx);
        if (Spec.IsValid())
        {
            Spec.Data->SetSetByCallerMagnitude(StudyTags::Data_Damage, DamageAmount);
            SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
        }

        // 적중 시 상태이상 부여(설정된 GE만, 이번 스윙이 허용될 때만 — 콤보는 마지막 타)
        if (bApplyStatusThisSwing)
        {
            for (const TSubclassOf<UGameplayEffect>& StatusGE : OnHitStatusEffects)
            {
                if (StatusGE == nullptr)
                {
                    continue;
                }
                FGameplayEffectContextHandle StatusCtx = SourceASC->MakeEffectContext();
                StatusCtx.AddSourceObject(Avatar);
                FGameplayEffectSpecHandle StatusSpec = SourceASC->MakeOutgoingSpec(StatusGE, 1.f, StatusCtx);
                if (StatusSpec.IsValid())
                {
                    SourceASC->ApplyGameplayEffectSpecToTarget(*StatusSpec.Data.Get(), TargetASC);
                }
            }
        }

        // 넉백(런치 이벤트 없을 때만)
        if (Feel.KnockbackSpeed > 0.f && EventOnHit.IsValid() == false)
        {
            if (ACharacter* TargetChar = Cast<ACharacter>(Target))
            {
                FVector KnockDir = (Target->GetActorLocation() - Avatar->GetActorLocation());
                KnockDir.Z = 0.f;
                KnockDir = KnockDir.GetSafeNormal();
                TargetChar->LaunchCharacter(KnockDir * Feel.KnockbackSpeed, true, false);
            }
        }

        // 히트스톱 대상에 피격자 추가
        if (Feel.HitStopDuration > 0.f)
        {
            Target->CustomTimeDilation = Feel.HitStopTimeDilation;
            StoppedActors.Add(Target);
        }

        if (AlreadyHit != nullptr)
        {
            AlreadyHit->Add(Target);
        }

        bHitAny = true;
    }

    if (bHitAny == false)
    {
        return false;
    }

    // ── 타격감: 히트스톱(공격자 포함) + 카메라 셰이크 ──────────────
    if (Feel.HitStopDuration > 0.f)
    {
        Avatar->CustomTimeDilation = Feel.HitStopTimeDilation;
        StoppedActors.Add(Avatar);

        FTimerHandle StopHandle;
        World->GetTimerManager().SetTimer(
            StopHandle,
            FTimerDelegate::CreateWeakLambda(Avatar, [StoppedActors]()
            {
                for (const TWeakObjectPtr<AActor>& Weak : StoppedActors)
                {
                    if (Weak.IsValid())
                    {
                        Weak->CustomTimeDilation = 1.f;
                    }
                }
            }),
            Feel.HitStopDuration, false);
    }

    if (Feel.CameraShake != nullptr)
    {
        if (APawn* AvatarPawn = Cast<APawn>(Avatar))
        {
            if (APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController()))
            {
                PC->ClientStartCameraShake(Feel.CameraShake);
            }
        }
    }

    return bHitAny;
}

// ── 노티파이 기반 타격 윈도우 ────────────────────────────────────────────────

void UCombatGameplayAbility::StartMeleeHitWindowListeners()
{
    // HitStart/HitEnd를 어빌 종료까지 계속 수신(콤보 전체 커버)
    if (UAbilityTask_WaitGameplayEvent* StartTask =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, StudyTags::Event_Melee_HitStart, nullptr, /*OnlyTriggerOnce=*/false))
    {
        StartTask->EventReceived.AddDynamic(this, &UCombatGameplayAbility::OnMeleeHitStartEvent);
        StartTask->ReadyForActivation();
    }
    if (UAbilityTask_WaitGameplayEvent* EndTask =
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, StudyTags::Event_Melee_HitEnd, nullptr, /*OnlyTriggerOnce=*/false))
    {
        EndTask->EventReceived.AddDynamic(this, &UCombatGameplayAbility::OnMeleeHitEndEvent);
        EndTask->ReadyForActivation();
    }
}

void UCombatGameplayAbility::OnMeleeHitStartEvent(FGameplayEventData /*Payload*/)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    // 서버에서 원격 폰(autonomous proxy)은 HitStart가 임팩트 구간 내내 매 프레임 재발생한다.
    // MeleeWindowOpenTime = "마지막 HitStart 수신 시각"으로 매번 갱신 → 틱이 끊김(>0.05s)을 보고
    // 임팩트 구간 종료를 판정한다. 한 임팩트 구간(=한 스윙) 진입 시에만 중복방지 셋을 리셋해,
    // 노티파이가 몇 번 울리든 한 스윙 = 데미지·넉백·히트스톱 각 1회가 된다.
    MeleeWindowOpenTime = World->GetTimeSeconds();

    if (bMeleeWindowOpen)
    {
        return;   // 이미 같은 임팩트 구간 진행 중 — 타이머/리셋 중복 방지
    }
    bMeleeWindowOpen = true;
    MeleeSwingHitActors.Reset();   // 새 임팩트 구간(새 스윙) — 여기서만 리셋
    World->GetTimerManager().SetTimer(
        MeleeWindowTimer, this, &UCombatGameplayAbility::MeleeWindowTick, 0.016f, true);
    MeleeWindowTick();   // 시작 지연 없이 즉시 1회
}

void UCombatGameplayAbility::OnMeleeHitEndEvent(FGameplayEventData /*Payload*/)
{
    // 노티파이 End는 서버에서 매 프레임 재발생할 수 있어 신뢰 불가 → 무시.
    // 윈도우는 MeleeWindowTick이 HitStart 끊김(>0.05s)을 감지해 스스로 닫는다.
}

void UCombatGameplayAbility::ResetMeleeSwingHits()
{
    MeleeSwingHitActors.Reset();
}

void UCombatGameplayAbility::MeleeWindowTick()
{
    // HitStart가 0.05s 넘게 끊겼으면 임팩트 구간 종료 → 윈도우 닫기(다음 스윙 HitStart가 다시 연다)
    UWorld* World = GetWorld();
    if (World != nullptr && bMeleeWindowOpen
        && (World->GetTimeSeconds() - MeleeWindowOpenTime) > 0.05)
    {
        StopMeleeHitWindow();
        return;
    }

    const bool bNewHit = ApplyMeleeDamage(MeleeDamage, MeleeHitEventTag, MeleeHitEventMagnitude, MeleeHitFeel, &MeleeSwingHitActors);
    if (bNewHit)
    {
        OnMeleeHitLanded();
    }
}

void UCombatGameplayAbility::StopMeleeHitWindow()
{
    bMeleeWindowOpen = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MeleeWindowTimer);
    }
}
