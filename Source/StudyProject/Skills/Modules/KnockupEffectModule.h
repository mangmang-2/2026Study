#pragma once

#include "CoreMinimal.h"
#include "Skills/EffectModule.h"
#include "KnockupEffectModule.generated.h"

/** 띄우기 모듈 — HitActors를 수직으로 띄운다(공중 콤보 셋업용). */
UCLASS(meta = (DisplayName = "Knockup (띄우기)"))
class STUDYPROJECT_API UKnockupEffectModule : public UEffectModule
{
    GENERATED_BODY()

public:
    virtual void Execute(const FSkillExecutionContext& Ctx) override;
    virtual FText GetSummary() const override;

    // 위로 띄우는 속도
    UPROPERTY(EditAnywhere, Category = "Knockup")
    float LaunchUpSpeed = 800.f;

    // 띄우면서 판정 중심으로 모으는 수평 속도(0=순수 수직)
    UPROPERTY(EditAnywhere, Category = "Knockup")
    float GatherToCenter = 0.f;
};
