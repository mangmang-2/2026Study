#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "GA_AirLaunch.generated.h"

class UAnimMontage;

/**
 * 적 공중 띄우기 반응 어빌리티.
 * Event.Launched 게임플레이 이벤트로 자동 트리거되어 캐릭터를 위로 발사하고
 * 공중 피격 몽타주를 재생, 착지(LandedDelegate) 시 넉다운(쓰러짐) 몽타주를 재생한다.
 */
UCLASS()
class STUDYPROJECT_API UGA_AirLaunch : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_AirLaunch();

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
    // 공중 피격(떠 있는 동안) 몽타주
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> AirHitMontage = nullptr;

    // 착지 후 넉다운(쓰러짐) 몽타주
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> KnockdownMontage = nullptr;

    // 넉다운 후 일어서는 몽타주 (없으면 바로 종료)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UAnimMontage> GetUpMontage = nullptr;

    // 이벤트에서 매그니튜드가 안 오면 사용할 기본 상승 속도
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float DefaultLaunchZ = 700.f;

    // 띄울 때(상승)는 정상 중력으로 솟구치고, 정점부터는 낮은 중력으로 체공한다.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Gravity")
    float LaunchGravityScale = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Gravity")
    float HangGravityScale = 0.35f;

    UFUNCTION()
    void OnLanded(const FHitResult& Hit);

    UFUNCTION()
    void OnKnockdownFinished();

    UFUNCTION()
    void OnGetUpFinished();

private:
    // 상승은 정상 중력 → 정점(Velocity.Z<=0)부터 체공 중력으로 전환 → 착지/종료 시 복원
    void ApplyLaunchGravity();
    void CheckApex();
    void RestoreGravity();

    float SavedGravityScale = 1.0f;
    bool  bGravityActive = false;
    bool  bGetUpStarted = false;   // 기상 몽타주 중복 시작 방지(넉다운 인터럽트 재진입 가드)
    FTimerHandle ApexTimerHandle;
};
