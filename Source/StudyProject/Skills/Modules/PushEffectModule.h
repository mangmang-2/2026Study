#pragma once

#include "CoreMinimal.h"
#include "Skills/EffectModule.h"
#include "PushEffectModule.generated.h"

/** 밀기 모듈 — HitActors를 시전자/중심점 반대 방향으로 밀어낸다. */
UCLASS(meta = (DisplayName = "Push (밀기)"))
class STUDYPROJECT_API UPushEffectModule : public UEffectModule
{
    GENERATED_BODY()

public:
    virtual void Execute(const FSkillExecutionContext& Ctx) override;
    virtual FText GetSummary() const override;

    // 밀어내는 수평 속도
    UPROPERTY(EditAnywhere, Category = "Push")
    float PushStrength = 1000.f;

    // 띄우는 상승 속도(0=수평만)
    UPROPERTY(EditAnywhere, Category = "Push")
    float UpwardBias = 0.f;

    // 시전자 위치 대신 판정 중심(Origin) 기준으로 밀어낼지(지점 폭발의 방사형 넉백)
    UPROPERTY(EditAnywhere, Category = "Push")
    bool bPushFromOrigin = true;
};
