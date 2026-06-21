#include "BTTask_StrafeAroundTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/EnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/StudyGameplayTags.h"

// 적의 블록 자세 플래그 on/off — ABP가 이 값을 읽어 애님 레이어를 전환한다.
static void SetEnemyBlockStance(UBehaviorTreeComponent& OwnerComp, bool bOn)
{
    if (AAIController* C = OwnerComp.GetAIOwner())
    {
        if (AEnemyCharacter* E = Cast<AEnemyCharacter>(C->GetPawn()))
        {
            E->SetBlockStance(bOn);
        }
    }
}

// 스트레이프 중엔 진행방향 자동회전(OrientRotationToMovement)을 끄고 타겟을 직접 응시한다.
// 끝나면 다시 켜 순찰/추격이 이동방향으로 회전하게 한다.
static void SetOrientToMovement(UBehaviorTreeComponent& OwnerComp, bool bEnable)
{
    if (AAIController* C = OwnerComp.GetAIOwner())
    {
        if (ACharacter* Ch = Cast<ACharacter>(C->GetPawn()))
        {
            if (UCharacterMovementComponent* M = Ch->GetCharacterMovement())
            {
                M->bOrientRotationToMovement = bEnable;
            }
        }
    }
}

UBTTask_StrafeAroundTarget::UBTTask_StrafeAroundTarget()
{
    NodeName = TEXT("Strafe Around Target");
    bNotifyTick = true;
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_StrafeAroundTarget, TargetKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_StrafeAroundTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    FBTStrafeMemory* Mem = reinterpret_cast<FBTStrafeMemory*>(NodeMemory);
    Mem->Remaining = FMath::FRandRange(MinDuration, MaxDuration);
    Mem->Side = (FMath::RandBool() ? 1.f : -1.f);   // 좌/우 랜덤
    Mem->bMoving = true;
    Mem->bCommitting = false;
    Mem->CommitElapsed = 0.f;
    Mem->PhaseTimer = FMath::FRandRange(0.5f, 1.0f); // 첫 걷기 구간
    SetEnemyBlockStance(OwnerComp, true);            // 전투 블록 자세 ON (ABP가 레이어 전환)
    return EBTNodeResult::InProgress;
}

void UBTTask_StrafeAroundTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    FBTStrafeMemory* Mem = reinterpret_cast<FBTStrafeMemory*>(NodeMemory);

    AAIController* AICon = OwnerComp.GetAIOwner();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    APawn* Pawn = AICon ? AICon->GetPawn() : nullptr;
    const FName Key = TargetKey.SelectedKeyName.IsNone() ? FName(TEXT("TargetActor")) : TargetKey.SelectedKeyName;
    AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(Key)) : nullptr;

    if (Pawn == nullptr || Target == nullptr)
    {
        SetOrientToMovement(OwnerComp, true);
        SetEnemyBlockStance(OwnerComp, false);
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);   // 타겟 잃음/폰 없음
        return;
    }

    // 피격/스태거/넉다운 중이면 견제를 멈추고 블록 자세를 꺼서 피격 몽타주가 온전히 보이게 한다.
    // (블록 레이어가 켜져 있으면 상체가 블록으로 덮여 피격 모션이 안 보임)
    if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
    {
        if (ASC->HasMatchingGameplayTag(StudyTags::State_HitReact)
            || ASC->HasMatchingGameplayTag(StudyTags::Status_Staggered)
            || ASC->HasMatchingGameplayTag(StudyTags::State_Knockdown))
        {
            SetEnemyBlockStance(OwnerComp, false);
            return;   // 이번 틱은 이동/회전/타이머 진행 없이 피격 반응에 양보
        }
    }
    // 피격이 아니면 전투 블록 자세 유지(피격으로 꺼졌던 것 복구)
    SetEnemyBlockStance(OwnerComp, true);

    const FVector SelfLoc = Pawn->GetActorLocation();
    FVector ToTarget = Target->GetActorLocation() - SelfLoc;
    ToTarget.Z = 0.f;
    const float Dist = ToTarget.Size();
    const FVector ToDir = (Dist > KINDA_SMALL_NUMBER) ? (ToTarget / Dist) : Pawn->GetActorForwardVector();

    // 전투 태세: 항상 플레이어를 바라본다(옆걸음 = 정면 응시 + 측면 이동).
    SetOrientToMovement(OwnerComp, false);
    const FRotator TargetRot(0.f, ToDir.Rotation().Yaw, 0.f);
    Pawn->SetActorRotation(FMath::RInterpTo(Pawn->GetActorRotation(), TargetRot, DeltaSeconds, TurnSpeed));

    auto Move = [&](const FVector& Dir, float Scale)
    {
        if (ACharacter* Char = Cast<ACharacter>(Pawn)) { Char->AddMovementInput(Dir, Scale); }
        else { Pawn->AddMovementInput(Dir, Scale); }
    };

    // 간보기 시간이 끝나면 돌입(거리 좁히기) 페이즈로
    Mem->Remaining -= DeltaSeconds;
    if (Mem->Remaining <= 0.f)
    {
        Mem->bCommitting = true;
    }

    if (Mem->bCommitting)
    {
        // 돌입: 플레이어를 향해 전진(옆걸음하다 앞으로). 사거리 안에 들면 공격으로.
        if (Dist <= AttackRange)
        {
            SetEnemyBlockStance(OwnerComp, false);
            FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
            return;
        }
        // 일정 시간 못 따라잡으면 간보기로 복귀(카이팅 추격 포기)
        Mem->CommitElapsed += DeltaSeconds;
        if (Mem->CommitElapsed >= CommitTimeout)
        {
            Mem->bCommitting = false;
            Mem->CommitElapsed = 0.f;
            Mem->Remaining = FMath::FRandRange(MinDuration, MaxDuration);   // 간보기 재시작
            Mem->bMoving = true;
            Mem->PhaseTimer = FMath::FRandRange(0.5f, 1.0f);
            return;
        }
        Move(ToDir, ApproachScale);   // 걷기~뛰기 중간 속도로 접근
        return;
    }

    // 간보기: 걷기↔멈춤 리듬 + 가끔 방향 전환, StrafeRadius 유지하며 측면 이동
    Mem->PhaseTimer -= DeltaSeconds;
    if (Mem->PhaseTimer <= 0.f)
    {
        Mem->bMoving = !Mem->bMoving;
        if (Mem->bMoving)
        {
            Mem->PhaseTimer = FMath::FRandRange(0.5f, 1.1f);
            if (FMath::FRand() < 0.45f) { Mem->Side *= -1.f; }
        }
        else
        {
            Mem->PhaseTimer = FMath::FRandRange(0.4f, 0.9f);
        }
    }

    if (Mem->bMoving)
    {
        const FVector Tangent = FVector::CrossProduct(FVector::UpVector, ToDir) * Mem->Side;
        const float RadiusError = Dist - StrafeRadius;          // +면 너무 멈, -면 너무 가까움
        const FVector Radial = ToDir * FMath::Clamp(RadiusError / 80.f, -1.f, 1.f);
        const FVector MoveDir = (Tangent + Radial).GetSafeNormal();
        Move(MoveDir, MoveScale);
    }
}
