#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "ComboData.h"
#include "GA_BossCharge.generated.h"

class UAnimMontage;
class UMaterialInterface;
class UDecalComponent;

/**
 * 보스 돌진(러시) 공격 — 텔레그래프형 패턴.
 *  1) 텔레그래프: 플레이어 방향으로 돌진 경로를 잡고, 바닥에 워닝 데칼 표시 + 윈드업 몽타주.
 *     이 시간 동안 플레이어가 피할 수 있음(방향은 발동 순간 고정).
 *  2) 돌진: 고정된 방향으로 빠르게 전진하며 경로상 대상에 1회씩 타격(ApplyMeleeDamage 재사용).
 *  3) 마무리: 스키드/회복 몽타주 후 종료.
 * EnemyCombatController의 AttackAbilities에 넣으면 AI가 사거리에서 랜덤 선택해 사용.
 * 몽타주/데칼 머티리얼은 비워도(nullable) 동작 — 에셋은 점진 배선.
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
    // 워닝 데칼이 보스→끝점까지 자라는 시간 = 텔레그래프(이 동안 방향 고정, 플레이어 회피 가능). 다 자라면 돌진.
    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float TelegraphTime = 4.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    TObjectPtr<UAnimMontage> WindupMontage;

    // 바닥 워닝 데칼 머티리얼(비우면 데칼 없이 동작)
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
