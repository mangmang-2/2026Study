#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_StatusBurning.generated.h"

/**
 * 상태이상: 화상(화염 지속 데미지).
 * HasDuration + Period로 일정 시간 동안 주기적으로 Damage meta attribute에 화염 피해를 더한다.
 * 적용 동안 Status.Burning 태그를 대상에 부여(VFX/UI/중복 판정용, GE 만료 시 자동 제거).
 */
UCLASS()
class STUDYPROJECT_API UGE_StatusBurning : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_StatusBurning();

    // 태그 부여 컴포넌트는 생성자가 아닌 여기서 추가(생성자 내 NewObject 금지)
    virtual void PostInitProperties() override;
};
