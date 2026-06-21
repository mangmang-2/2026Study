#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_DetectPlayer.generated.h"

/**
 * 시야 범위 + 직선 시야(LOS) 안의 가장 가까운 플레이어를 TargetActor 블랙보드 키에 세팅.
 * 이미 타겟이 있으면 LoseRadius를 벗어날 때 해제(추격 유지/이탈).
 */
UCLASS()
class STUDYPROJECT_API UBTService_DetectPlayer : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_DetectPlayer();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    // 타겟을 저장할 블랙보드 키(Object)
    UPROPERTY(EditAnywhere, Category = "AI")
    struct FBlackboardKeySelector TargetKey;

    // 이 거리 안 + 시야면 발견
    UPROPERTY(EditAnywhere, Category = "AI")
    float SightRadius = 1500.f;

    // 타겟이 이 거리를 벗어나면 잃음(추격 포기)
    UPROPERTY(EditAnywhere, Category = "AI")
    float LoseRadius = 2200.f;

    // 발견에 직선 시야(벽 가림 없음) 요구
    UPROPERTY(EditAnywhere, Category = "AI")
    bool bRequireLineOfSight = true;

private:
    bool HasLineOfSight(class AActor* From, class AActor* To) const;
};
