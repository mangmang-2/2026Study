#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "ComboData.h"
#include "GA_JustCounter.generated.h"

class UAnimMontage;

/**
 * 저스트카운터(패리) 어빌리티.
 * Input.Parry로 활성화 → 짧은 패리 윈도우(Status.Parrying) 동안 적 공격을 정면에서 막으면
 * ApplyMeleeDamage가 데미지를 무효화하고 Event.Parried를 보내준다.
 * Event.Parried를 받으면 리포스트(카운터) 몽타주를 재생해 경직된 적에게 큰 데미지를 준다.
 * 윈도우 안에 막지 못하면(헛패리) 그냥 종료.
 */
UCLASS()
class STUDYPROJECT_API UGA_JustCounter : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_JustCounter();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

protected:
    // 패리 자세(짧은 가드) 몽타주
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> ParryStanceMontage = nullptr;

    // 패리 성공 시 리포스트(카운터 공격) 몽타주 — "Melee Hit" 노티파이로 데미지 판정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> CounterMontage = nullptr;

    // 패리 유효 시간(초). 활성화 직후 이 시간 동안만 Status.Parrying 유지("저스트" 타이밍)
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.05"))
    float ParryWindow = 0.3f;

    // 리포스트 데미지
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float CounterDamage = 80.f;

    // 리포스트 타격감
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    FHitFeel CounterHitFeel;

private:
    UFUNCTION()
    void OnParrySuccess(FGameplayEventData Payload);

    UFUNCTION()
    void OnStanceFinished();

    UFUNCTION()
    void OnCounterFinished();

    void CloseParryWindow();

    bool bCountering = false;
    FTimerHandle ParryWindowTimer;
};
