#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CombatGameplayAbility.generated.h"

class UGameplayEffect;
struct FHitFeel;

/**
 * 모든 전투 GA의 베이스.
 * - InputTag: 이 어빌리티를 활성화하는 입력 태그(Input.*)
 * - bActivateOnGranted: 부여 즉시 활성화(패시브용)
 */
UCLASS(Abstract)
class STUDYPROJECT_API UCombatGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UCombatGameplayAbility();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Input")
    FGameplayTag InputTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    bool bActivateOnGranted = false;

    // true면 이 어빌리티가 활성 중인 동안 캐릭터 이동 입력을 막는다(공격/회피/처형 등 모션 동작)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    bool bLocksMovement = false;

    virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
    // ── 근접 데미지 (공격 GA 공용) ──────────────────────────────────
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage")
    TSubclassOf<UGameplayEffect> DamageGEClass;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    float MeleeRange = 175.f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    float MeleeRadius = 70.f;

    // 전방 스피어 트레이스 → 맞은 ASC들에 DamageGEClass(SetByCaller Data.Damage=DamageAmount) 적용.
    // EventOnHit이 유효하면 각 타깃 ASC로 게임플레이 이벤트도 전송.
    // Feel(타격감): 적중 지점 이펙트 + 히트스톱(공격자+피격자) + 카메라 셰이크 + 넉백. 하나라도 맞으면 true.
    bool ApplyMeleeDamage(float DamageAmount, FGameplayTag EventOnHit, float EventMagnitude, const FHitFeel& Feel);
};
