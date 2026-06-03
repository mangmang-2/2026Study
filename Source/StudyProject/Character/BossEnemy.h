#pragma once

#include "CoreMinimal.h"
#include "Character/EnemyCharacter.h"
#include "BossEnemy.generated.h"

struct FOnAttributeChangeData;
class UUserWidget;

/**
 * 보스 적. AEnemyCharacter를 확장 — 큰 체력 + HP 비율 기반 페이즈 전환(이동속도 강화 등) +
 * 화면 상단 보스 체력바. 공격 종류/AI는 BP(컨트롤러 AttackAbilities) + DefaultAbilities로 구성.
 */
UCLASS()
class STUDYPROJECT_API ABossEnemy : public AEnemyCharacter
{
    GENERATED_BODY()

public:
    ABossEnemy();

    virtual void BeginPlay() override;

protected:
    // 페이즈 전환 HP 비율(내림차순). 예: {0.66, 0.33} → 2회 전환(총 3페이즈)
    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    TArray<float> PhaseHealthThresholds;

    // 페이즈 전환마다 더해지는 이동속도(보스가 점점 공격적으로)
    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    float PerPhaseSpeedBonus = 80.f;

    // 보스 체력바 위젯 클래스(기본: UBossHealthBarWidget 코드 위젯)
    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    TSubclassOf<UUserWidget> BossHealthBarClass;

    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    FText BossName;

    // 페이즈 전환 시 연출(BP에서 포효 몽타주/이펙트 등)
    UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
    void OnPhaseChanged(int32 NewPhase);

private:
    void HandleHPChanged(const FOnAttributeChangeData& Data);

    int32 CurrentPhase = 0;

    UPROPERTY()
    TObjectPtr<UUserWidget> BossHealthBarWidget = nullptr;
};
