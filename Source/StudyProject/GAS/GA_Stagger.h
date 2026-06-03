#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "GA_Stagger.generated.h"

class UAnimMontage;

/**
 * 패리당한 공격자(적)의 경직 어빌리티.
 * Event.Staggered 게임플레이 이벤트로 자동 트리거되어 스태거(가드브레이크) 몽타주를 재생한다.
 * 진행 중인 공격 몽타주를 끊어 큰 빈틈(처형 오프닝)을 만든다.
 * 재생 동안 Status.Staggered를 부여(처형/추가타 판정용).
 */
UCLASS()
class STUDYPROJECT_API UGA_Stagger : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Stagger();

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
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> StaggerMontage = nullptr;

    UFUNCTION()
    void OnMontageFinished();
};
