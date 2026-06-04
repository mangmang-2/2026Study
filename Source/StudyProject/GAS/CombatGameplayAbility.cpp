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

// 공격범위 시각화 옵션 — 콘솔에서 `study.DrawMeleeRange 1`(켜기) / `0`(끄기)
static TAutoConsoleVariable<int32> CVarDrawMeleeRange(
    TEXT("study.DrawMeleeRange"),
    0,
    TEXT("Draw melee attack range (sweep) debug shape. 0=off, 1=on"),
    ECVF_Default);

UCombatGameplayAbility::UCombatGameplayAbility()
{
    // 콤보/회피 등은 인스턴스별 상태가 필요
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 기본은 로컬 예측 활성화(서버 권위 + 클라 예측)
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

    const FVector Start = Avatar->GetActorLocation();
    const FVector End = Start + Avatar->GetActorForwardVector() * MeleeRange;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Avatar);

    TArray<FHitResult> Hits;
    World->SweepMultiByChannel(
        Hits, Start, End, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeSphere(MeleeRadius), Params);

#if ENABLE_DRAW_DEBUG
    // 공격범위 시각화(옵션) — 스피어 스윕을 캡슐로 그림. study.DrawMeleeRange 1 로 켬.
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

    // 아군 오사 방지용 공격자 진영(적=AEnemyCharacter/보스, 그 외=플레이어 진영)
    const bool bAvatarIsEnemy = Avatar->IsA(AEnemyCharacter::StaticClass());

    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (Target == nullptr || Done.Contains(Target))
        {
            continue;
        }
        Done.Add(Target);

        // 같은 진영(적↔적 / 플레이어↔플레이어)은 데미지 안 줌 — 아군 오사 방지
        if (Target->IsA(AEnemyCharacter::StaticClass()) == bAvatarIsEnemy)
        {
            continue;
        }

        // 한 스윙(여러 프레임) 동안 같은 대상 중복 타격 방지
        if (AlreadyHit != nullptr && AlreadyHit->Contains(Target))
        {
            continue;
        }

        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
        if (TargetASC == nullptr)
        {
            continue;
        }

        // 이미 죽었거나 쓰러진(넉다운) 대상은 더 이상 때리지 않음
        if (TargetASC->HasMatchingGameplayTag(StudyTags::State_Dead)
            || TargetASC->HasMatchingGameplayTag(StudyTags::State_Knockdown))
        {
            continue;
        }

        // ── 저스트카운터(패리) 가로채기 ────────────────────────────────
        // 타깃이 패리 윈도우 중이고 공격자를 바라보고 있으면: 데미지/넉백/히트스톱 전부 무효 +
        // 공격자에게 Event.Staggered(경직) + 패리한 쪽에 Event.Parried(리포스트 발동) 전송.
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
                // 패리한 쪽(플레이어) 성공 — 리포스트 GA가 수신
                {
                    FGameplayEventData ParryP;
                    ParryP.EventTag = StudyTags::Event_Parried;
                    ParryP.Instigator = Avatar;
                    ParryP.Target = Target;
                    TargetASC->HandleGameplayEvent(StudyTags::Event_Parried, &ParryP);
                }
                // 이 스윙에서 같은 대상 재판정 방지(데미지는 안 줌)
                if (AlreadyHit != nullptr)
                {
                    AlreadyHit->Add(Target);
                }
                continue;   // 데미지/넉백/히트스톱/피드백 모두 건너뜀
            }
        }

        // 타격 피드백(히트VFX + 피격자 플래시 + 데미지넘버) — 멀티에서 다른 유저도 보이게
        // 공격자 액터의 NetMulticast로 재생(서버 권위에서만 트리거 → 모든 클라). VFX는 null이어도 OK.
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

        // 런치 등 이벤트 먼저 전송(피격 반응보다 우선되도록)
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

        // 넉백 — 피격자를 공격자 반대 방향으로 밀기(런치 이벤트가 없을 때만)
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
    // HitStart/HitEnd 이벤트를 어빌리티가 끝날 때까지 계속 수신(여러 타 콤보 전부 커버)
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
    // 새 스윙 시작 — 중복방지 셋 초기화 후 윈도우 동안 매 프레임 트레이스
    MeleeSwingHitActors.Reset();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            MeleeWindowTimer, this, &UCombatGameplayAbility::MeleeWindowTick, 0.016f, true);
        MeleeWindowTick();   // 같은 프레임에 즉시 1회 판정(시작 지연 없이)
    }
}

void UCombatGameplayAbility::OnMeleeHitEndEvent(FGameplayEventData /*Payload*/)
{
    StopMeleeHitWindow();
}

void UCombatGameplayAbility::MeleeWindowTick()
{
    const bool bNewHit = ApplyMeleeDamage(MeleeDamage, MeleeHitEventTag, MeleeHitEventMagnitude, MeleeHitFeel, &MeleeSwingHitActors);
    if (bNewHit)
    {
        OnMeleeHitLanded();
    }
}

void UCombatGameplayAbility::StopMeleeHitWindow()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MeleeWindowTimer);
    }
}
