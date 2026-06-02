#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "GA_Executed.generated.h"

class UAnimMontage;

/**
 * 처형 피해자 반응 어빌리티(적).
 * Event.Executed 게임플레이 이벤트로 자동 트리거되어 피해자 몽타주를 재생하고,
 * 끝나면 사망(래그돌) 처리한다.
 */
UCLASS()
class STUDYPROJECT_API UGA_Executed : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Executed();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    // 피해자(당하는 쪽) 몽타주 (Execution_Target 계열)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> ExecutedMontage = nullptr;

    UFUNCTION()
    void OnMontageFinished();
};
