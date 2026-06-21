#include "BTTask_EnemyAttack.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

UBTTask_EnemyAttack::UBTTask_EnemyAttack()
{
    NodeName = TEXT("Enemy Attack");
    bNotifyTick = true;
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyAttack, TargetKey), AActor::StaticClass());
}

static bool IsAnyMontagePlayingOn(APawn* Pawn)
{
    if (ACharacter* Char = Cast<ACharacter>(Pawn))
    {
        if (USkeletalMeshComponent* Mesh = Char->GetMesh())
        {
            if (UAnimInstance* Anim = Mesh->GetAnimInstance())
            {
                return Anim->IsAnyMontagePlaying();
            }
        }
    }
    return false;
}

EBTNodeResult::Type UBTTask_EnemyAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AICon = OwnerComp.GetAIOwner();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    APawn* Pawn = AICon ? AICon->GetPawn() : nullptr;
    if (Pawn == nullptr || AttackAbility == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    // 공격 직전 타겟 응시(있으면)
    if (BB != nullptr)
    {
        const FName Key = TargetKey.SelectedKeyName.IsNone() ? FName(TEXT("TargetActor")) : TargetKey.SelectedKeyName;
        if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(Key)))
        {
            FVector ToTarget = Target->GetActorLocation() - Pawn->GetActorLocation();
            ToTarget.Z = 0.f;
            if (ToTarget.SizeSquared() > KINDA_SMALL_NUMBER)
            {
                Pawn->SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
            }
        }
    }

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
    if (ASC == nullptr || ASC->TryActivateAbilityByClass(AttackAbility) == false)
    {
        return EBTNodeResult::Failed;
    }

    FBTAttackMemory* Mem = reinterpret_cast<FBTAttackMemory*>(NodeMemory);
    Mem->bSawMontage = false;
    Mem->Elapsed = 0.f;
    return EBTNodeResult::InProgress;
}

void UBTTask_EnemyAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    FBTAttackMemory* Mem = reinterpret_cast<FBTAttackMemory*>(NodeMemory);
    AAIController* AICon = OwnerComp.GetAIOwner();
    APawn* Pawn = AICon ? AICon->GetPawn() : nullptr;
    if (Pawn == nullptr)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    Mem->Elapsed += DeltaSeconds;

    const bool bPlaying = IsAnyMontagePlayingOn(Pawn);
    if (bPlaying)
    {
        Mem->bSawMontage = true;
    }

    // 몽타주가 한 번 재생된 뒤 끝났으면 완료. 안전망(MaxWait)도 둠.
    if ((Mem->bSawMontage && bPlaying == false) || Mem->Elapsed >= MaxWait)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}
