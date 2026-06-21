#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPatrolPoint.generated.h"

/**
 * HomeLocation 주변 PatrolRadius 내의 navmesh 위 랜덤 지점을 찾아 PatrolLocation 키에 세팅.
 * (이후 BT의 MoveTo가 그 지점으로 이동)
 */
UCLASS()
class STUDYPROJECT_API UBTTask_FindPatrolPoint : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FindPatrolPoint();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
    // 순찰 중심(Vector) — 컨트롤러가 스폰 위치로 세팅
    UPROPERTY(EditAnywhere, Category = "AI")
    struct FBlackboardKeySelector HomeKey;

    // 찾은 순찰 지점을 저장할 키(Vector)
    UPROPERTY(EditAnywhere, Category = "AI")
    struct FBlackboardKeySelector PatrolLocationKey;

    // 순찰 반경
    UPROPERTY(EditAnywhere, Category = "AI")
    float PatrolRadius = 800.f;
};
