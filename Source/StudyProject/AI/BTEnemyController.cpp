#include "BTEnemyController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

ABTEnemyController::ABTEnemyController()
{
    // 컨트롤러가 폰 회전을 직접 제어(적 캐릭터가 bUseControllerRotationYaw=false라 영향 없지만 명시)
}

void ABTEnemyController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (BehaviorTreeAsset == nullptr || InPawn == nullptr)
    {
        return;
    }

    // BT 실행(블랙보드는 BT 에셋에 지정된 것 사용)
    if (RunBehaviorTree(BehaviorTreeAsset))
    {
        // 순찰 중심 = 스폰 위치
        if (UBlackboardComponent* BB = GetBlackboardComponent())
        {
            BB->SetValueAsVector(HomeLocationKey, InPawn->GetActorLocation());
        }
    }
}
