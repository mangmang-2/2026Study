#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "ComboData.h"
#include "GA_BossCharge.generated.h"

class UAnimMontage;
class UMaterialInterface;
class UDecalComponent;

/**
 * 보스 돌진 공격(텔레그래프형): 텔레그래프(방향 고정·워닝 데칼) → 돌진(경로 타격) → 마무리.
 * EnemyCombatController.AttackAbilities에 넣으면 AI가 사거리에서 선택해 사용. 몽타주/데칼은 비워도 동작.
 */
UCLASS()
class STUDYPROJECT_API UGA_BossCharge : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_BossCharge();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

protected:
    // ── 텔레그래프 ──────────────────────────────────────────────────
    // 데칼이 다 자라는 시간 = 텔레그래프(방향 고정, 회피 가능)
    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float TelegraphTime = 4.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    TObjectPtr<UAnimMontage> WindupMontage;

    // 비우면 데칼 없이 동작
    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    TObjectPtr<UMaterialInterface> WarningDecalMaterial;

    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float DecalWidth = 220.f;

    // ── 돌진 ────────────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float ChargeDistance = 900.f;

    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float ChargeTime = 0.55f;

    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    TObjectPtr<UAnimMontage> ChargeMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float ChargeDamage = 35.f;

    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    FHitFeel ChargeHitFeel;

    // ── 마무리 ──────────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    TObjectPtr<UAnimMontage> EndMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float RecoverTime = 0.6f;

private:
    void BeginCharge();   // 텔레그래프 종료 → 돌진 시작
    void ChargeTick();    // 16ms마다 전진 + 경로 타격
    void EndCharge();     // 돌진 종료 → 마무리 후 EndAbility

    FVector ChargeDir = FVector::ForwardVector;
    FVector ChargeStartLoc = FVector::ZeroVector;
    FVector ChargeEndLoc = FVector::ZeroVector;
    float ChargeElapsed = 0.f;

    FTimerHandle PhaseTimer;       // 텔레그래프/회복 단계 전환
    FTimerHandle ChargeTickTimer;  // 돌진 이동/타격 틱

    // 한 번의 돌진에서 같은 대상 중복 타격 방지
    TSet<TWeakObjectPtr<AActor>> ChargeHitActors;
};
