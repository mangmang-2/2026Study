#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CombatGameplayAbility.generated.h"

/**
 * 모든 전투 GA의 베이스.
 * - InputTag: 이 어빌리티를 활성화하는 입력 태그(Input.*)
 * - bActivateOnGranted: 부여 즉시 활성화(패시브용)
 */
UCLASS(Abstract)
class STUDYPROJECT_API UCombatGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UCombatGameplayAbility();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Input")
    FGameplayTag InputTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    bool bActivateOnGranted = false;

    virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};
