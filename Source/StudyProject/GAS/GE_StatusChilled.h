#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_StatusChilled.generated.h"

/**
 * 상태이상: 둔화(이동속도 감소).
 * HasDuration 동안 Status.Chilled 태그를 대상에 부여한다(데미지 없음).
 * CharacterBase 태그 콜백이 MaxWalkSpeed에 둔화 배율을 곱한다(GE 만료 시 복원).
 */
UCLASS()
class STUDYPROJECT_API UGE_StatusChilled : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_StatusChilled();

    // 태그 부여 컴포넌트는 생성자가 아닌 여기서 추가(생성자 내 NewObject 금지)
    virtual void PostInitProperties() override;
};
