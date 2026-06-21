#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyAttack.generated.h"

class UGameplayAbility;

/** 공격 태스크 노드별 상태 */
struct FBTAttackMemory
{
    bool  bSawMontage = false;   // 공격 몽타주가 한 번이라도 재생됐는지
    float Elapsed = 0.f;
};

/**
 * 기존 적 공격 GA(GA_EnemyAttack 등)를 발동하고, 공격 몽타주가 끝날 때까지 대기 후 Succeeded.
 * 발동 실패 시 Failed. (전투 로직은 기존 GAS 그대로 재사용)
 */
UCLASS()
class STUDYPROJECT_API UBTTask_EnemyAttack : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_EnemyAttack();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTAttackMemory); }

protected:
    // 응시할 타겟(Object) — 공격 직전 타겟을 바라봄
    UPROPERTY(EditAnywhere, Category = "AI")
    struct FBlackboardKeySelector TargetKey;

    // 발동할 공격 어빌리티(BP_GA_EnemyAttack 등). 적이 이미 grant한 클래스여야 함.
    UPROPERTY(EditAnywhere, Category = "AI")
    TSubclassOf<UGameplayAbility> AttackAbility;

    // 몽타주가 안 끝나도 이 시간이 지나면 강제 종료(안전망)
    UPROPERTY(EditAnywhere, Category = "AI")
    float MaxWait = 3.0f;
};
