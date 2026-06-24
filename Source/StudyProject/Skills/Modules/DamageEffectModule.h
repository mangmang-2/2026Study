#pragma once

#include "CoreMinimal.h"
#include "Skills/EffectModule.h"
#include "DamageEffectModule.generated.h"

class UGameplayEffect;

/** 데미지 모듈 — HitActors에 데미지 GE를 적용한다(SetByCaller Data.Damage). */
UCLASS(meta = (DisplayName = "Damage (데미지)"))
class STUDYPROJECT_API UDamageEffectModule : public UEffectModule
{
    GENERATED_BODY()

public:
    UDamageEffectModule();

    virtual void Execute(const FSkillExecutionContext& Ctx) override;
    virtual FText GetSummary() const override;

    UPROPERTY(EditAnywhere, Category = "Damage")
    float DamageAmount = 60.f;

    // 비우면 기본 데미지 GE(GE_Damage) 사용
    UPROPERTY(EditAnywhere, Category = "Damage")
    TSubclassOf<UGameplayEffect> DamageGEClass;
};
