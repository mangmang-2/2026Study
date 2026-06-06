#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "GA_EnemyAttack.generated.h"

class UAnimMontage;

/**
 * 적 근접 공격 어빌리티(AI가 발동).
 * 공격 몽타주를 재생하고, 몽타주의 "Melee Hit" 노티파이 윈도우에서 전방을 트레이스해
 * 플레이어에게 데미지를 준다. 몽타주의 휘두르기 전 구간이 곧 텔레그래프(선딜).
 */
UCLASS()
class STUDYPROJECT_API UGA_EnemyAttack : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_EnemyAttack();

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
    // 단일 공격 몽타주(ComboMontages가 비었을 때 사용)
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> AttackMontage;

    // 채우면 한 발동에서 1→2→3→4를 크로스블렌드로 연결
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TArray<TObjectPtr<UAnimMontage>> ComboMontages;

    // 다음 타로 넘어가는 시점(현재 타 길이 비율). 작을수록 빠릿.
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.2", ClampMax = "1.0"))
    float ChainFraction = 0.55f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float AttackDamage = 10.f;

    // 몽타주 재생 속도(<1이면 느린 강공격=텔레그래프, >1이면 빠른 공격)
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float AttackPlayRate = 1.0f;

    // 타격감(넉백 등) — 적 공격용
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    FHitFeel AttackHitFeel;

private:
    void PlayChainStep();      // 현재 스텝 몽타주 재생 + 다음/종료 예약

    int32 ComboStep = 0;
    FTimerHandle ChainTimer;
};
