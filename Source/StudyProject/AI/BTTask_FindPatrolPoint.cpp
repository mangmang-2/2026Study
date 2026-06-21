#include "BTTask_FindPatrolPoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

UBTTask_FindPatrolPoint::UBTTask_FindPatrolPoint()
{
    NodeName = TEXT("Find Patrol Point");
    HomeKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolPoint, HomeKey));
    PatrolLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolPoint, PatrolLocationKey));
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AICon = OwnerComp.GetAIOwner();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    APawn* Pawn = AICon ? AICon->GetPawn() : nullptr;
    if (Pawn == nullptr || BB == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    // 순찰은 이동방향으로 회전(스트레이프가 껐을 수 있으니 복원)
    if (ACharacter* Ch = Cast<ACharacter>(Pawn))
    {
        if (UCharacterMovementComponent* M = Ch->GetCharacterMovement())
        {
            M->bOrientRotationToMovement = true;
        }
    }

    const FName HKey = HomeKey.SelectedKeyName.IsNone() ? FName(TEXT("HomeLocation")) : HomeKey.SelectedKeyName;
    const FName PKey = PatrolLocationKey.SelectedKeyName.IsNone() ? FName(TEXT("PatrolLocation")) : PatrolLocationKey.SelectedKeyName;

    // 순찰 중심: HomeKey가 있으면 사용, 없으면 현재 위치
    FVector Center = Pawn->GetActorLocation();
    if (BB->IsVectorValueSet(HKey))
    {
        Center = BB->GetValueAsVector(HKey);
    }

    UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
    if (Nav == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    FNavLocation Result;
    if (Nav->GetRandomReachablePointInRadius(Center, PatrolRadius, Result) == false)
    {
        return EBTNodeResult::Failed;
    }

    BB->SetValueAsVector(PKey, Result.Location);
    return EBTNodeResult::Succeeded;
}
