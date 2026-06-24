#pragma once

#include "CoreMinimal.h"
#include "Skills/EffectModule.h"
#include "SlowEffectModule.generated.h"

class UGameplayEffect;

/** 둔화 모듈 — HitActors에 둔화 상태이상 GE를 적용한다. 기존 GE_StatusChilled 재사용. */
UCLASS(meta = (DisplayName = "Slow (둔화)"))
class STUDYPROJECT_API USlowEffectModule : public UEffectModule
{
    GENERATED_BODY()

public:
    USlowEffectModule();

    virtual void Execute(const FSkillExecutionContext& Ctx) override;
    virtual FText GetSummary() const override;

    // 적용할 둔화 GE(배율은 GE 정의에 baked). 기본 = 둔화.
    UPROPERTY(EditAnywhere, Category = "Slow")
    TSubclassOf<UGameplayEffect> StatusGEClass;

    // 둔화 지속시간(초). 0이면 GE 정의값 사용. >0이면 이 값으로 덮어씀(지속시간 증가).
    UPROPERTY(EditAnywhere, Category = "Slow")
    float SlowDuration = 0.f;
};
