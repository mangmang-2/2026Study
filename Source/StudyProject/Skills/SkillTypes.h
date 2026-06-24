#pragma once

#include "CoreMinimal.h"
#include "SkillTypes.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/** 어디를 향하나 — 타겟팅 방식 */
UENUM(BlueprintType)
enum class ESkillTargetingMode : uint8
{
    NoTarget        UMETA(DisplayName = "NoTarget (자기 기준)"),
    PointTarget     UMETA(DisplayName = "Point (마우스 지점)"),
    DirectionTarget UMETA(DisplayName = "Direction (정면/조준 방향)"),
    ActorTarget     UMETA(DisplayName = "Actor (록온 타겟)"),
};

/** 어떻게 닿나 — 전달 방식 */
UENUM(BlueprintType)
enum class ESkillDeliveryType : uint8
{
    AOE         UMETA(DisplayName = "AOE (지점 범위 폭발)"),
    Cone        UMETA(DisplayName = "Cone (정면 부채꼴)"),
    Projectile  UMETA(DisplayName = "Projectile (투사체)"),
    Beam        UMETA(DisplayName = "Beam (직선 관통)"),
    Melee       UMETA(DisplayName = "Melee (근접 즉시)"),
    Dash        UMETA(DisplayName = "Dash (돌진 판정)"),
};

/** 모듈 실행 순서 */
UENUM(BlueprintType)
enum class ESkillExecutionMode : uint8
{
    Simultaneous UMETA(DisplayName = "Simultaneous (동시)"),
    Sequential   UMETA(DisplayName = "Sequential (순차)"),
};

/**
 * 스킬 1회 실행의 공유 컨텍스트.
 * 판정은 한 번만 하고, 모든 EffectModule이 같은 HitActors를 순서대로 처리한다.
 * 서버 권위에서 채워져 모듈로 전달된다.
 */
USTRUCT()
struct STUDYPROJECT_API FSkillExecutionContext
{
    GENERATED_BODY()

    // 시전자(아바타)
    UPROPERTY()
    TObjectPtr<AActor> Instigator = nullptr;

    // 시전자 ASC(데미지/상태이상 GE의 소스)
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> InstigatorASC = nullptr;

    // 스킬을 실행한 GA(이펙트 컨텍스트 핸들 생성용)
    UPROPERTY()
    TObjectPtr<UGameplayAbility> SourceAbility = nullptr;

    // 판정 기준점(폭발 중심 / 투사체 착탄 지점)
    FVector Origin = FVector::ZeroVector;

    // 판정 방향(부채꼴/빔/투사체 진행 방향)
    FVector Direction = FVector::ForwardVector;

    // 판정 결과 — 모든 모듈이 공유
    TArray<TWeakObjectPtr<AActor>> HitActors;
};
