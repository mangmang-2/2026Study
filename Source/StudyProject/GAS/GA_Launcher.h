#pragma once

#include "CoreMinimal.h"
#include "CombatGameplayAbility.h"
#include "ComboData.h"
#include "GA_Launcher.generated.h"

class UDataTable;

/**
 * 런처(공중 띄우기) 어빌리티.
 * Input.Launcher로 활성화. 무기 데이터의 LauncherMontage를 재생하고,
 * 적중 시 적에게 Event.Launched 전송(적이 공중에 뜸) + 플레이어도 살짝 띄워 공중 콤보로 연계.
 */
UCLASS()
class STUDYPROJECT_API UGA_Launcher : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Launcher();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TObjectPtr<UDataTable> ComboDataTable;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    FName DefaultComboRow = TEXT("Default");

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
    float HitDelay = 0.2f;

    // 플레이어 자기 자신을 띄울 때도 상승은 정상 중력, 정점부터 체공 중력으로.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Gravity")
    float LaunchGravityScale = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Gravity")
    float HangGravityScale = 0.35f;

    UFUNCTION()
    void OnMontageFinished();

    UFUNCTION()
    void DoMeleeTrace();

    UFUNCTION()
    void OnSelfLanded(const FHitResult& Hit);

private:
    // 상승은 정상 중력 → 정점부터 체공 중력 → 착지 시 복원
    void ApplyLaunchGravity();
    void CheckApex();
    void RestoreGravity();

    float CurrentDamage = 25.f;

    // 이번 활성화의 띄우는 높이·재생 속도(데이터에서 캐시)
    float CurrentLaunchEnemyZ = 700.f;
    float CurrentLaunchSelfZ = 600.f;
    float CurrentPlayRate = 1.0f;

    float SavedGravityScale = 1.0f;
    bool  bGravityActive = false;
    FTimerHandle ApexTimerHandle;

    UPROPERTY()
    FHitFeel CurrentHitFeel;
};
