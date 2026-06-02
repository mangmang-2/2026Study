#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "GA_HitReact.generated.h"

class UAnimMontage;

/**
 * 피격 반응 어빌리티.
 * Event.HitReact 게임플레이 이벤트로 자동 트리거되어 피격 몽타주를 재생한다.
 */
UCLASS()
class STUDYPROJECT_API UGA_HitReact : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_HitReact();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> HitMontage = nullptr;

    UFUNCTION()
    void OnMontageFinished();
};
