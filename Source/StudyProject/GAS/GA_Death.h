#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "GA_Death.generated.h"

class UAnimMontage;

/**
 * 사망 반응 어빌리티.
 * Event.Death 게임플레이 이벤트로 자동 트리거되어 사망 몽타주를 재생하고,
 * 끝나면 래그돌로 전환한다. (이동/입력 비활성)
 */
UCLASS()
class STUDYPROJECT_API UGA_Death : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Death();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> DeathMontage = nullptr;

    UFUNCTION()
    void OnDeathMontageFinished();
};
