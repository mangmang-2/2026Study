#pragma once

#include "CoreMinimal.h"
#include "Skills/EffectModule.h"
#include "PullEffectModule.generated.h"

/** 당기기 모듈 — HitActors를 시전자/중심점 쪽으로 끌어당긴다. */
UCLASS(meta = (DisplayName = "Pull (당기기)"))
class STUDYPROJECT_API UPullEffectModule : public UEffectModule
{
    GENERATED_BODY()

public:
    virtual void Execute(const FSkillExecutionContext& Ctx) override;
    virtual FText GetSummary() const override;

    // 끌어당기는 수평 속도
    UPROPERTY(EditAnywhere, Category = "Pull")
    float PullStrength = 1200.f;

    // 살짝 띄우는 상승 속도(0=수평만)
    UPROPERTY(EditAnywhere, Category = "Pull")
    float UpwardBias = 0.f;

    // 시전자 위치 대신 판정 중심(Origin)으로 끌어당길지
    UPROPERTY(EditAnywhere, Category = "Pull")
    bool bPullToOrigin = false;
};
