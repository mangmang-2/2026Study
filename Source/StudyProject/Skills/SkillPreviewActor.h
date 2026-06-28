#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkillPreviewActor.generated.h"

class USkillDefinition;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 에디터 뷰포트에서 스킬 비주얼을 시퀀스 재생(게임플레이 없이).
 * 시전(CastVFX) → 전달(투사체 비행 / AOE 착탄 / 낙뢰 시간차 낙하) → 착탄(ImpactVFX) 순으로 보여준다.
 * ShouldTickIfViewportsOnly로 PIE 없이 에디터 리얼타임에서 틱한다. 재생이 끝나면 스스로 파괴.
 */
UCLASS()
class STUDYPROJECT_API ASkillPreviewActor : public AActor
{
    GENERATED_BODY()

public:
    ASkillPreviewActor();

    void InitPreview(USkillDefinition* InSkill, const FVector& InCenter, const FVector& InCasterLoc);

    virtual bool ShouldTickIfViewportsOnly() const override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void Destroyed() override;

private:
    void BeginDelivery();
    UNiagaraComponent* SpawnVFX(UNiagaraSystem* System, const FVector& Location, float Scale, bool bAutoDestroy);
    float GroundZ(const FVector& Point) const;

    // 이 프리뷰가 스폰한 VFX — 액터 파괴 시 함께 정리
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraComponent>> SpawnedVFX;

    UPROPERTY()
    TObjectPtr<USkillDefinition> Skill = nullptr;

    FVector Center = FVector::ZeroVector;
    FVector CasterLoc = FVector::ZeroVector;

    float Elapsed = 0.f;
    bool bCastDone = false;
    bool bDelivered = false;
    float FinishTime = 0.f;

    UPROPERTY()
    TObjectPtr<UNiagaraComponent> ProjectileComp = nullptr;
    FVector ProjectilePos = FVector::ZeroVector;
    bool bProjectileActive = false;

    int32 RainSpawned = 0;
    float RainNextTime = 0.f;
    bool bRainActive = false;
};
