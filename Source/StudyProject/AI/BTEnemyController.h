#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BTEnemyController.generated.h"

class UBehaviorTree;

/**
 * BehaviorTree 기반 적 AI 컨트롤러.
 * 빙의 시 지정한 BT를 실행하고, 블랙보드 HomeLocation을 스폰 위치로 세팅(순찰 중심).
 * 공격/돌진 등 전투는 기존 GA(GA_EnemyAttack)를 BT 태스크가 발동해 재사용한다.
 */
UCLASS()
class STUDYPROJECT_API ABTEnemyController : public AAIController
{
    GENERATED_BODY()

public:
    ABTEnemyController();

protected:
    virtual void OnPossess(APawn* InPawn) override;

    // 실행할 BehaviorTree (BP/CDO에서 지정)
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

    // 블랙보드 키 이름(BB 에셋의 키와 일치해야 함)
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    FName HomeLocationKey = TEXT("HomeLocation");
};
