#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ComboData.h"
#include "CombatGameplayAbility.generated.h"

class UGameplayEffect;

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

    // 활성 중 캐릭터 이동 입력 차단(공격/회피 등 모션)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    bool bLocksMovement = false;

    virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
    // ── 근접 데미지 (공격 GA 공용) ──────────────────────────────────
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage")
    TSubclassOf<UGameplayEffect> DamageGEClass;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    float MeleeRange = 135.f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    float MeleeRadius = 70.f;

    // 적중 시 부여할 상태이상 GE(비어 있으면 없음)
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Status")
    TArray<TSubclassOf<UGameplayEffect>> OnHitStatusEffects;

    // 정면 패리 판정 내적 임계값(0.1 ≈ 약 84도)
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    float ParryFacingDot = 0.1f;

    // 전방 스피어 트레이스로 데미지 GE 적용 + 타격감(이펙트/히트스톱/셰이크/넉백).
    // EventOnHit 유효 시 이벤트도 전송. AlreadyHit로 한 스윙 중복 방지. 새 명중 있으면 true.
    bool ApplyMeleeDamage(float DamageAmount, FGameplayTag EventOnHit, float EventMagnitude, const FHitFeel& Feel,
        TSet<TWeakObjectPtr<AActor>>* AlreadyHit = nullptr);

    // ── 노티파이 기반 타격 윈도우 ───────────────────────────────────
    // 서브클래스가 Melee* 파라미터를 채운 뒤 호출. "Melee Hit" 노티파이의 HitStart/HitEnd로
    // 윈도우를 열고/닫아 그동안 매 프레임 트레이스한다.
    void StartMeleeHitWindowListeners();
    void StopMeleeHitWindow();

    // 윈도우 중 새 대상이 맞은 프레임 훅(자기 체공 등)
    virtual void OnMeleeHitLanded() {}

    // 이번 스윙 파라미터(서브클래스가 채움)
    float MeleeDamage = 0.f;
    FGameplayTag MeleeHitEventTag;
    float MeleeHitEventMagnitude = 0.f;
    UPROPERTY()
    FHitFeel MeleeHitFeel;

    // 이번 스윙에 OnHitStatusEffects를 적용할지(콤보가 마지막 타만 켜는 용도). 기본 true.
    bool bApplyStatusThisSwing = true;

private:
    UFUNCTION()
    void OnMeleeHitStartEvent(FGameplayEventData Payload);
    UFUNCTION()
    void OnMeleeHitEndEvent(FGameplayEventData Payload);
    void MeleeWindowTick();

    TSet<TWeakObjectPtr<AActor>> MeleeSwingHitActors;
    FTimerHandle MeleeWindowTimer;
};
