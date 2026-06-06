#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CombatAbilitySystemComponent.generated.h"

/**
 * 프로젝트 ASC 서브클래스.
 * 입력 태그로 GA를 활성화하는 헬퍼 제공.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class STUDYPROJECT_API UCombatAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    // 입력 태그와 일치하는 GA 활성화(Enhanced Input 핸들러에서 호출)
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryActivateAbilityByInputTag(FGameplayTag InputTag);

    // 활성 GA 중 bLocksMovement가 있으면 true
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool IsMovementLocked() const;
};
