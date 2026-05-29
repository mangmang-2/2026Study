#include "AICharacterController.h"
#include "AICharacterBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

const FName AAICharacterController::KeyTarget          = TEXT("TargetActor");
const FName AAICharacterController::KeySelfActor       = TEXT("SelfActor");
const FName AAICharacterController::KeyPatrolLocation  = TEXT("PatrolLocation");

AAICharacterController::AAICharacterController()
{
    // BlackboardComponent는 AAIController가 이미 생성 — 중복 생성 없음
}

void AAICharacterController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    AAICharacterBase* AIChar = Cast<AAICharacterBase>(InPawn);
    if (!AIChar) return;

    UBehaviorTree* SelectedBT = AIChar->IsHostile() ? CombatBT : PatrolBT;
    if (!SelectedBT) return;

    UBlackboardComponent* BB = nullptr;
    if (UseBlackboard(SelectedBT->BlackboardAsset, BB))
    {
        BB->SetValueAsObject(KeySelfActor, InPawn);
        RunBehaviorTree(SelectedBT);
    }
}

void AAICharacterController::OnUnPossess()
{
    Super::OnUnPossess();

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsObject(KeyTarget,         nullptr);
        BB->SetValueAsObject(KeySelfActor,      nullptr);
        BB->SetValueAsVector(KeyPatrolLocation, FVector::ZeroVector);
    }
}

void AAICharacterController::SetTargetActor(AActor* Target)
{
    if (UBlackboardComponent* BB = GetBlackboardComponent())
        BB->SetValueAsObject(KeyTarget, Target);
}

AActor* AAICharacterController::GetTargetActor() const
{
    if (const UBlackboardComponent* BB = GetBlackboardComponent())
        return Cast<AActor>(BB->GetValueAsObject(KeyTarget));
    return nullptr;
}
