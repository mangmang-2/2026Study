#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "GA_Finisher.generated.h"

class UAnimMontage;
class UDataTable;

/**
 * 처형(Execution) 어빌리티 — 테스트용 단축키 발동(HP 조건 없음).
 * Input.Finisher로 활성화. 앞에 있는 적을 찾아 플레이어 앞으로 워프시켜 마주보게 한 뒤
 * 플레이어 처형 몽타주를 재생하고 적에게 Event.Executed를 전송(적은 피해자 애니 후 사망).
 */
UCLASS()
class STUDYPROJECT_API UGA_Finisher : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Finisher();

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
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TObjectPtr<UDataTable> ComboDataTable;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    FName DefaultComboRow = TEXT("Default");

    // 처형 대상 탐색 범위
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher")
    float FinisherRange = 250.f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher")
    float FinisherRadius = 90.f;

    // 처형 시 적을 플레이어 앞 이 거리에 워프(두 캐릭터 사이 간격)
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher")
    float WarpDistance = 150.f;

    // ── 연출 ────────────────────────────────────────────────
    // 처형 중 글로벌 시간 배율(슬로모션). 1=정상
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher")
    float SlowMoScale = 0.5f;

    // 연출 카메라 블렌드 시간(초)
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher")
    float CameraBlendTime = 0.25f;

    // ── 연출 카메라 위치/각도(전부 BP에서 조절) ──────────────
    // 두 캐릭터 옆으로 떨어진 거리(클수록 멀리서)
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher|Camera")
    float CameraSideDistance = 200.f;

    // 두 캐릭터 축에서 카메라쪽으로 당기는 정도(앞/뒤 보정)
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher|Camera")
    float CameraBackOffset = 40.f;

    // 카메라 높이(두 캐릭터 중점 기준 위로). 작을수록 눈높이, 클수록 위에서
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher|Camera")
    float CameraHeight = 70.f;

    // 카메라가 바라보는 지점 높이(중점 기준 위로). 캐릭터 상체쯤
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher|Camera")
    float CameraLookAtHeight = 80.f;

    // 시야각(FOV). 작을수록 클로즈업(망원), 클수록 넓게
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Finisher|Camera", meta = (ClampMin = "20.0", ClampMax = "120.0"))
    float CameraFOV = 55.f;

    UFUNCTION()
    void OnMontageFinished();

private:
    // 연출용 카메라 시작/종료(슬로모션 + 시점 전환)
    void StartCinematic(AActor* Target);
    void EndCinematic();

    UPROPERTY()
    TObjectPtr<AActor> CineCamera = nullptr;
};
