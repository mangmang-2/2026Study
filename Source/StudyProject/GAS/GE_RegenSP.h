#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_RegenSP.generated.h"

/**
 * SP 자동 회복 GE (무한 주기). 캐릭터 시작 시 적용.
 * 0.5초마다 SP +5 회복(상시). 공격 소모량이 더 커서 공격 중엔 순감소.
 */
UCLASS()
class STUDYPROJECT_API UGE_RegenSP : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_RegenSP();
};
