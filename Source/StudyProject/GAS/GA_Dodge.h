#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "GA_Dodge.generated.h"

class UAnimMontage;

/**
 * 8방향 회피 어빌리티.
 * Input.Dodge로 활성화. 이동 입력 방향을 캐릭터 정면 기준 8방향으로 매핑해
 * 해당 회피 몽타주를 재생하고 그 방향으로 이동한다.
 * (록온 중엔 캐릭터가 타겟을 바라보므로 타겟 기준 8방향 회피가 됨)
 */
UCLASS()
class STUDYPROJECT_API UGA_Dodge : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Dodge();

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
    // 8방향 회피 몽타주 (순서: F, FR, R, BR, B, BL, L, FL)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TArray<TObjectPtr<UAnimMontage>> DodgeMontages;

    // 회피 이동 거리(고정). 속도+마찰 방식은 얼음처럼 미끄러져서, 정해진 거리를
    // 짧은 시간에 ease-out으로 이동 후 딱 멈추는 보간 방식으로 변경(지상/공중 동일, 절제된 정지).
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float DodgeDistance = 600.f;

    // 이동 시간을 회피 몽타주 길이의 몇 배로 할지(1.0=애니와 동시 종료). 이동이 먼저 끝나
    // 애니만 남는 렉 느낌 방지 — 몽타주 길이에 맞춰 이동.
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float DodgeMoveFraction = 0.55f;

    UFUNCTION()
    void OnMontageFinished();

private:
    // 회피 이동 보간(타이머)
    void DodgeMoveTick();
    void StopDodgeMove();

    FTimerHandle DodgeMoveTimer;
    FVector DodgeStartLoc = FVector::ZeroVector;
    FVector DodgeEndLoc   = FVector::ZeroVector;
    float   DodgeElapsed  = 0.f;
    float   CurrentMoveDuration = 0.18f;   // 활성화 시 몽타주 길이로 설정
};
