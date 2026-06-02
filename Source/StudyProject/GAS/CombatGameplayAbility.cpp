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

bool UCombatGameplayAbility::ApplyMeleeDamage(float DamageAmount, FGameplayTag EventOnHit, float EventMagnitude, const FHitFeel& Feel)
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
