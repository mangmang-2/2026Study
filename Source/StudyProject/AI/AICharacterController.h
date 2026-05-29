#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AICharacterController.generated.h"

class AAICharacterBase;
class UBehaviorTree;
class UBlackboardComponent;

UCLASS()
class STUDYPROJECT_API AAICharacterController : public AAIController
{
    GENERATED_BODY()

public:
    AAICharacterController();

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    void SetTargetActor(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "AI")
    AActor* GetTargetActor() const;

    // Blackboard 키 이름
    static const FName KeyTarget;
    static const FName KeySelfActor;
    static const FName KeyPatrolLocation;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    TObjectPtr<UBehaviorTree> CombatBT;

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    TObjectPtr<UBehaviorTree> PatrolBT;

};
