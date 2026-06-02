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
    // Feel(타격감): 적중 지점 이펙트 + 히트스톱(공격자+피격자) + 카메라 셰이크 + 넉백.
    // AlreadyHit: 지정 시 그 안에 있는 액터는 건너뛰고, 새로 맞은 액터를 추가(한 스윙 다중프레임 중복방지).
    // 반환: 이번 호출에서 새로 맞은 대상이 하나라도 있으면 true.
    bool ApplyMeleeDamage(float DamageAmount, FGameplayTag EventOnHit, float EventMagnitude, const FHitFeel& Feel,
        TSet<TWeakObjectPtr<AActor>>* AlreadyHit = nullptr);

    // ── 노티파이 기반 타격 윈도우 ───────────────────────────────────
    // 서브클래스가 ActivateAbility에서 Melee* 파라미터를 채운 뒤 호출.
    // 몽타주의 "Melee Hit" 노티파이가 Event.Melee.HitStart/HitEnd를 보내면 윈도우를 열고/닫아
    // 윈도우 동안 매 프레임 트레이스한다(스윙마다 타이밍이 달라도 정확히 판정).
    void StartMeleeHitWindowListeners();
    void StopMeleeHitWindow();

    // 윈도우 중 "새 대상이 맞은" 프레임에 호출(자기 체공/자기 런치 등 후처리 훅)
    virtual void OnMeleeHitLanded() {}

    // 이번 스윙에 적용할 파라미터(서브클래스가 채움)
    float MeleeDamage = 0.f;
    FGameplayTag MeleeHitEventTag;
    float MeleeHitEventMagnitude = 0.f;
    UPROPERTY()
    FHitFeel MeleeHitFeel;

private:
    UFUNCTION()
    void OnMeleeHitStartEvent(FGameplayEventData Payload);
    UFUNCTION()
    void OnMeleeHitEndEvent(FGameplayEventData Payload);
    void MeleeWindowTick();

    TSet<TWeakObjectPtr<AActor>> MeleeSwingHitActors;
    FTimerHandle MeleeWindowTimer;
};
