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
#include "Camera/CameraShakeBase.h"
#include "TimerManager.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

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

    bool bHitAny = false;
    TSet<AActor*> Done;
    TArray<TWeakObjectPtr<AActor>> StoppedActors;   // 히트스톱 복원 대상
    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (Target == nullptr || Done.Contains(Target))
        {
            continue;
        }
        Done.Add(Target);

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

        // 적중 지점에 히트 이펙트 스폰(설정된 경우)
        if (Feel.HitEffect != nullptr)
        {
            const FVector ImpactPoint(Hit.ImpactPoint);
            const FVector FxLoc = ImpactPoint.IsZero() ? Target->GetActorLocation() : ImpactPoint;
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
