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
    // 입력 태그(Input.*)와 일치하는 GA를 활성화 (Enhanced Input 핸들러에서 호출)
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryActivateAbilityByInputTag(FGameplayTag InputTag);

    // 활성 중인 어빌리티 중 bLocksMovement=true인 것이 하나라도 있으면 true(이동 입력 차단용)
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool IsMovementLocked() const;
};
