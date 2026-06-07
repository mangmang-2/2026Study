#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "ComboData.h"
#include "GA_Combo.generated.h"

class UAnimMontage;
class UDataTable;

/**
 * 지상 콤보 어빌리티 — 무기별 DataTable 기반. 한 번의 활성화로 콤보 전체를 관리한다.
 * 공격 중 같은 입력이 다시 오면 다음 타를 버퍼에 예약 → 블렌드아웃 시점에 바로 다음 타로 이어붙임.
 * 공중 콤보는 이 클래스를 상속해 SelectCombo만 오버라이드.
 */
UCLASS()
class STUDYPROJECT_API UGA_Combo : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Combo();

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

    // 이미 활성 중인 콤보에 같은 입력이 다시 들어왔을 때 ASC가 호출 — 다음 타를 예약
    void NotifyComboInput();

protected:
    // 무기별 콤보 데이터 테이블 (행 키 = 무기 ItemID / Default)
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TObjectPtr<UDataTable> ComboDataTable;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    FName DefaultComboRow = TEXT("Default");

    // 몽타주 시작 후 히트 판정까지 지연
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    float HitDelay = 0.25f;

    // 적중 시 보낼 이벤트(공중 콤보=Event.Launched). 비우면 없음.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    FGameplayTag HitEventTag;

    // 이벤트 매그니튜드(공중 콤보=재상승 속도)
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    float HitEventMagnitude = 0.f;

    // 마지막 타 전용 이벤트(공중 콤보=Event.Slammed). 비우면 일반 타와 동일.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    FGameplayTag LastHitEventTag;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    float LastHitEventMagnitude = 0.f;

    // OnHitStatusEffects를 콤보 마지막 타에만 적용(피니셔 디버프). false면 모든 타.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Status")
    bool bStatusOnLastHitOnly = true;

    // ── 공중 콤보 자기 체공 ─────────────────────────────────────────────
    // 공중 타격 성공 시 플레이어 자신도 중력을 낮추고 살짝 다시 떠서 콤보 도중 안 떨어지게.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|AirFloat")
    bool bFloatSelfOnHit = false;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|AirFloat")
    float SelfHangGravityScale = 0.35f;

    // 타격마다 줄 수직 속도(0=낙하만 멈춤, 양수=살짝 상승)
    UPROPERTY(EditDefaultsOnly, Category = "Combat|AirFloat")
    float SelfPopZ = 0.f;

    // 캔슬 윈도우 오픈 시점(몽타주 길이 비율). 작을수록 빠릿, 클수록 또박또박.
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.1", ClampMax = "0.95"))
    float ComboWindowFraction = 0.5f;

    // 데이터 행에서 사용할 콤보 배열 선택 (지상=GroundCombo, 공중 콤보 클래스는 오버라이드)
    virtual const TArray<TObjectPtr<UAnimMontage>>& SelectCombo(const FWeaponComboData& Data) const;

    UFUNCTION()
    void OnComboWindowOpen();

    UFUNCTION()
    void OnComboBlendOut();

    UFUNCTION()
    void OnComboInterrupted();

    // 노티파이 타격 윈도우에서 새 적이 맞은 프레임 훅(공중 콤보 자기 체공 처리)
    virtual void OnMeleeHitLanded() override;

    // 공중 콤보 자기 체공 — 착지 시 중력 복원
    UFUNCTION()
    void OnSelfLanded(const FHitResult& Hit);

    // 스텝인 세부값은 DT_ComboData.StepIn으로. 여기엔 전진키 입력 임계값만(무기 무관).
    UPROPERTY(EditDefaultsOnly, Category = "Combat|StepIn")
    float StepInForwardThreshold = 0.3f;

private:
    // 현재 인덱스의 콤보 몽타주를 재생하고 히트 판정/캔슬 윈도우 타이머를 건다
    void PlayComboMontage();

    // 전진키가 눌려있고 타겟이 앞에 있으면 타겟 쪽으로 조금 접근(타격 시작 시 호출)
    void TryStepInToTarget();
    void StepInTick();
    void StopStepIn();
    AActor* FindStepInTarget(const FVector& SelfLoc, const FVector& Forward) const;

    // 자기 체공 적용/복원
    void ApplySelfFloat();
    void RestoreSelfGravity();

    // 버퍼된 입력이 있으면 다음 타로 진행(중복 진행 방지)
    void AdvanceCombo();

    // 이번 활성화에서 사용할 콤보 몽타주 배열(활성화 시점에 무기로 조회해 캐시)
    UPROPERTY()
    TArray<TObjectPtr<UAnimMontage>> CurrentMontages;

    // 현재 재생 중인 콤보 타 인덱스
    int32 ComboIndex = 0;

    // 다음 타 입력이 예약되었는지
    bool bInputBuffered = false;

    // 현재 타의 캔슬 윈도우가 열렸는지
    bool bWindowOpen = false;

    // 현재 타에서 이미 다음 타로 넘어갔는지(중복 진행 방지)
    bool bChained = false;

    // 우리가 의도적으로 몽타주를 교체하는 중인지(이때 발생하는 OnInterrupted는 무시)
    bool bAdvancing = false;

    float CurrentDamage = 25.f;

    // 이번 활성화의 공격 몽타주 재생 속도(데이터에서 캐시)
    float CurrentPlayRate = 1.0f;

    // 스텝인 보간 상태 + 이번 활성화의 스텝인 파라미터(DT에서 캐시)
    FComboStepIn CurrentStepIn;
    FTimerHandle StepInTimerHandle;
    FVector StepInStartLoc = FVector::ZeroVector;
    FVector StepInEndLoc = FVector::ZeroVector;
    FRotator StepInTargetRot = FRotator::ZeroRotator;   // 타겟을 바라보는 목표 회전(Yaw)
    float StepInElapsed = 0.f;

    // 자기 체공 상태
    float SavedSelfGravity = 1.0f;
    bool  bSelfFloatActive = false;

    UPROPERTY()
    FHitFeel CurrentHitFeel;
};
