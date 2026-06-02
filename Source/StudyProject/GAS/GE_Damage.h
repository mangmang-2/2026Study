#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Damage.generated.h"

/**
 * 근접 공격용 즉시(Instant) 데미지 GE.
 * SetByCaller(Data.Damage) 매그니튜드로 데미지량을 전달받아 Damage meta attribute에 더한다.
 * (AttributeSet의 PostGameplayEffectExecute가 Damage→HP로 환산)
 */
UCLASS()
class STUDYPROJECT_API UGE_Damage : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_Damage();
};
