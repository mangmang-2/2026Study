#pragma once

#include "CoreMinimal.h"
#include "Skills/EffectModule.h"
#include "StunEffectModule.generated.h"

class UGameplayEffect;

/** 스턴 모듈 — HitActors에 감전(스턴) 상태이상 GE를 적용한다. 기존 GE_StatusShocked 재사용. */
UCLASS(meta = (DisplayName = "Stun (스턴/감전)"))
class STUDYPROJECT_API UStunEffectModule : public UEffectModule
{
    GENERATED_BODY()

public:
    UStunEffectModule();

    virtual void Execute(const FSkillExecutionContext& Ctx) override;
    virtual FText GetSummary() const override;

    // 적용할 상태이상 GE. 기본 = 감전.
    UPROPERTY(EditAnywhere, Category = "Stun")
    TSubclassOf<UGameplayEffect> StatusGEClass;

    // 스턴 지속시간(초). 0이면 GE 정의값 사용. >0이면 이 값으로 덮어씀(지속시간 증가).
    UPROPERTY(EditAnywhere, Category = "Stun")
    float StunDuration = 0.f;
};
