#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyCombatController.generated.h"

class UGameplayAbility;

/**
 * GAS 적(AEnemyCharacter)용 간단 전투 AI 컨트롤러(BehaviorTree 미사용).
 * 매 틱: 플레이어를 찾아 바라보고, 사거리 밖이면 접근, 안이면 쿨다운마다 공격 어빌리티 발동.
 * 몽타주 재생 중(공격/피격/넉다운)에는 이동/공격하지 않는다.
 */
UCLASS()
class STUDYPROJECT_API AEnemyCombatController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyCombatController();

protected:
    virtual void Tick(float DeltaSeconds) override;

    // 이 거리 안에 들어와야 플레이어를 인식/추격
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float AcquireRange = 1800.f;

    // 이 거리 안이면 공격
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float AttackRange = 200.f;

    // 콤보 회복 시간(ComboMax회 연속공격 후 쉬는 시간)
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float AttackCooldown = 2.0f;

    // 연속공격 사이 짧은 간격(초)
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float ComboGap = 0.4f;

    // 한 번에 칠 공격(콤보) 횟수(이후 AttackCooldown 회복). AM_Combo_All이 이미 다타 콤보라 1 권장.
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    int32 ComboMax = 1;

    // 플레이어가 적보다 이만큼 위(공중)면 공격하지 않음
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float MaxAttackHeight = 170.f;

    // 플레이어 바라보는 회전 속도
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float TurnSpeed = 8.f;

    // 발동할 공격 어빌리티(기본 GA_EnemyAttack)
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    TSubclassOf<UGameplayAbility> AttackAbility;

    // 여러 공격(보스 등) — 비어있지 않으면 사거리 안에서 랜덤으로 하나 선택해 발동.
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    TArray<TSubclassOf<UGameplayAbility>> AttackAbilities;

    // ── 갭클로저(돌진 등) — 플레이어가 멀 때 거리 좁히는 스킬 ──────────
    // 지정 시: 플레이어가 [ChargeMinRange, ChargeMaxRange]면 접근 대신 이 스킬을 쓴다.
    UPROPERTY(EditDefaultsOnly, Category = "AI|Charge")
    TSubclassOf<UGameplayAbility> ChargeAbility;

    // 이보다 가까우면 돌진 안 함(근접은 평타). 이보다 멀면 돌진 사거리 밖이라 그냥 접근.
    UPROPERTY(EditDefaultsOnly, Category = "AI|Charge")
    float ChargeMinRange = 350.f;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Charge")
    float ChargeMaxRange = 1500.f;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Charge")
    float ChargeCooldown = 5.f;

    // 플레이어와 높이차가 이보다 크면 돌진 안 함(다른 층으로 수평 돌진하다 벽에 박는 것 방지)
    UPROPERTY(EditDefaultsOnly, Category = "AI|Charge")
    float ChargeMaxHeightDiff = 250.f;

private:
    float LastAttackTime = -1000.f;
    float LastChargeTime = -1000.f;
    int32 ComboCount = 0;
};
