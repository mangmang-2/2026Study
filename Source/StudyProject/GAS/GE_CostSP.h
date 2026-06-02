#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_CostSP.generated.h"

/**
 * 공격 SP 소모 GE (즉시). 공격 GA의 CostGameplayEffectClass로 사용 →
 * CommitAbility가 자동 적용하고, SP가 부족하면 발동을 막는다.
 */
UCLASS()
class STUDYPROJECT_API UGE_CostSP : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_CostSP();
};
