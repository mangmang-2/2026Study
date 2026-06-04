#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_StatusShocked.generated.h"

/**
 * 상태이상: 감전(짧은 스턴).
 * HasDuration 동안 Status.Shocked 태그를 대상에 부여한다(데미지 없음).
 * - 플레이어: HandleMove/입력 라우팅에서 Status.Shocked면 이동·공격 차단
 * - 적: EnemyCombatController가 Status.Shocked면 AI 완전 정지
 * - CharacterBase 태그 콜백이 MaxWalkSpeed를 0으로(스턴) 만듦
 */
UCLASS()
class STUDYPROJECT_API UGE_StatusShocked : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_StatusShocked();

    // 태그 부여 컴포넌트는 생성자가 아닌 여기서 추가(생성자 내 NewObject 금지)
    virtual void PostInitProperties() override;
};
