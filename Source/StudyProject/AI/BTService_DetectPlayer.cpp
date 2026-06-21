#include "BTService_DetectPlayer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Character/PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"

// 바닥에 감지(초록)/이탈(빨강) 범위 원 + 타겟선 표시. 콘솔: ai.DrawEnemySight 0/1
static TAutoConsoleVariable<int32> CVarDrawEnemySight(
    TEXT("ai.DrawEnemySight"), 1,
    TEXT("Draw enemy AI sight(green)/lose(red) range circles + target line. 0=off,1=on"),
    ECVF_Default);

UBTService_DetectPlayer::UBTService_DetectPlayer()
{
    NodeName = TEXT("Detect Player");
    Interval = 0.3f;       // 0.3초마다 감지(매 프레임 불필요)
    RandomDeviation = 0.05f;
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayer, TargetKey), AActor::StaticClass());
}

bool UBTService_DetectPlayer::HasLineOfSight(AActor* From, AActor* To) const
{
    if (From == nullptr || To == nullptr)
    {
        return false;
    }
    UWorld* World = From->GetWorld();
    if (World == nullptr)
    {
        return false;
    }
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(From);
    Params.AddIgnoredActor(To);
    const FVector Start = From->GetActorLocation() + FVector(0, 0, 60.f);
    const FVector End = To->GetActorLocation() + FVector(0, 0, 60.f);
    FHitResult Hit;
    // 정적 지오메트리에 막히면 시야 없음
    return World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params) == false;
}

void UBTService_DetectPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AICon = OwnerComp.GetAIOwner();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (AICon == nullptr || BB == nullptr)
    {
        return;
    }
    APawn* SelfPawn = AICon->GetPawn();
    if (SelfPawn == nullptr)
    {
        return;
    }
    const FVector SelfLoc = SelfPawn->GetActorLocation();

    // 빌드도구가 서비스 키 셀렉터를 못 묶는 경우 대비 — 비어있으면 "TargetActor"로 폴백
    const FName KeyName = TargetKey.SelectedKeyName.IsNone() ? FName(TEXT("TargetActor")) : TargetKey.SelectedKeyName;

    AActor* ResultTarget = nullptr;

    // 현재 타겟이 있으면 이탈(LoseRadius) 검사
    AActor* CurrentTarget = Cast<AActor>(BB->GetValueAsObject(KeyName));
    if (CurrentTarget != nullptr)
    {
        const float Dist = FVector::Dist(SelfLoc, CurrentTarget->GetActorLocation());
        if (Dist > LoseRadius)
        {
            BB->ClearValue(KeyName);   // 너무 멀어짐 → 추격 포기
        }
        else
        {
            ResultTarget = CurrentTarget;   // 타겟 유지
        }
    }
    else
    {
        // 타겟 없음 → 시야 안 가장 가까운 플레이어 탐색
        TArray<AActor*> Players;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), Players);

        AActor* Best = nullptr;
        float BestDistSq = SightRadius * SightRadius;
        for (AActor* P : Players)
        {
            if (P == nullptr)
            {
                continue;
            }
            const float DistSq = FVector::DistSquared(SelfLoc, P->GetActorLocation());
            if (DistSq > BestDistSq)
            {
                continue;
            }
            if (bRequireLineOfSight && HasLineOfSight(SelfPawn, P) == false)
            {
                continue;
            }
            BestDistSq = DistSq;
            Best = P;
        }

        if (Best != nullptr)
        {
            BB->SetValueAsObject(KeyName, Best);
            ResultTarget = Best;
        }
    }

#if ENABLE_DRAW_DEBUG
    // 바닥에 감지/이탈 범위 원 + (타겟 있으면) 타겟선
    if (CVarDrawEnemySight.GetValueOnGameThread() != 0)
    {
        UWorld* World = GetWorld();
        const FVector Ground = SelfLoc - FVector(0, 0, SelfPawn->GetSimpleCollisionHalfHeight() - 5.f);
        const FVector AxisX(1, 0, 0), AxisY(0, 1, 0);
        DrawDebugCircle(World, Ground, SightRadius, 48, FColor::Green, false, Interval + 0.1f, 0, 3.f, AxisX, AxisY, false);
        DrawDebugCircle(World, Ground, LoseRadius, 48, FColor(255, 80, 0), false, Interval + 0.1f, 0, 2.f, AxisX, AxisY, false);
        if (ResultTarget != nullptr)
        {
            DrawDebugLine(World, SelfLoc, ResultTarget->GetActorLocation(), FColor::Yellow, false, Interval + 0.1f, 0, 3.f);
        }
    }
#endif
}
