#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillTypes.h"
#include "SkillDefinition.generated.h"

class UEffectModule;
class UTexture2D;
class UNiagaraSystem;
class UAnimMontage;
class UMaterialInterface;

/**
 * 스킬 한 개의 데이터 정의(기획자 편집).
 * 타겟팅(어디를) × 전달(어떻게) 2축으로 닿는 방식을 정하고,
 * EffectModules 배열로 데미지/당기기/스턴 등 효과를 조합한다.
 */
UCLASS(BlueprintType)
class STUDYPROJECT_API USkillDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // ── 표시(UI) ────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    FText SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    TObjectPtr<UTexture2D> Icon;

    // ── 전달 방식 ───────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery")
    ESkillTargetingMode TargetingMode = ESkillTargetingMode::PointTarget;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery")
    ESkillDeliveryType DeliveryType = ESkillDeliveryType::AOE;

    // ── 시전/판정 ───────────────────────────────────────────────────
    // 0이면 즉시 발동
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast")
    float CastTime = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast")
    float Cooldown = 5.f;

    // ── 지속(필드)형 ────────────────────────────────────────────────
    // 0이면 단발(시전 완료 시 1회 폭발). >0이면 그 시간 동안 유지되며 TickInterval마다 재판정·모듈 재실행.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duration")
    float Duration = 0.f;

    // 지속 중 모듈 재실행 주기(초). 예: 1.0=1초에 1번, 0.8=0.8초마다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duration", meta = (ClampMin = "0.05"))
    float TickInterval = 1.0f;

    // PointTarget 사거리 / 투사체·빔 최대 도달거리
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
    float Range = 800.f;

    // AOE 폭발 반경 / Cone·Beam 의 폭
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
    float Radius = 300.f;

    // Cone 반각(도). DeliveryType=Cone일 때만 사용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "1", ClampMax = "180"))
    float ConeHalfAngle = 45.f;

    // ── 모듈 조합 ───────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modules")
    ESkillExecutionMode ExecutionMode = ESkillExecutionMode::Sequential;

    // Sequential일 때 모듈 사이 간격(초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modules", meta = (EditCondition = "ExecutionMode == ESkillExecutionMode::Sequential"))
    float SequentialStep = 0.15f;

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Modules")
    TArray<TObjectPtr<UEffectModule>> EffectModules;

    // ── VFX / 연출 ──────────────────────────────────────────────────
    // 착탄/폭발 VFX (AOE 중심·투사체 충돌 지점)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    TObjectPtr<UNiagaraSystem> ImpactVFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    float ImpactVFXScale = 1.f;

    // VFX가 제작된 기준 반경 — 이 반경에서 ImpactVFXScale 그대로 적용.
    // Radius가 커지면 VFX도 비례 확대(0이면 비례 끄고 ImpactVFXScale만 사용).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    float VFXReferenceRadius = 200.f;

    // 투사체 비행 VFX (DeliveryType=Projectile)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    TObjectPtr<UNiagaraSystem> ProjectileVFX;

    // 투사체 속도/충돌 반경 (DeliveryType=Projectile)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
    float ProjectileSpeed = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
    float ProjectileRadius = 16.f;

    // 시전 모션(없으면 제자리)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    TObjectPtr<UAnimMontage> CastMontage;

    // 타겟팅 프리뷰 데칼(PointTarget 홀드 시 바닥 표시)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    TObjectPtr<UMaterialInterface> RangeDecalMaterial;
};
