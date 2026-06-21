#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_StrafeAroundTarget.generated.h"

/** 스트레이프 태스크 노드별 상태(인스턴스 메모리) */
struct FBTStrafeMemory
{
    float Remaining = 0.f;    // 남은 간보기(feint) 시간
    float Side = 1.f;         // 회전 방향(좌/우)
    float PhaseTimer = 0.f;   // 현재 걷기/멈춤 페이즈 남은 시간
    bool  bMoving = true;     // true=걷는 중, false=멈춰 타이밍 보는 중
    bool  bCommitting = false;// true=거리 좁히려 전진 돌입(간보기 종료 후)
    float CommitElapsed = 0.f;// 돌입 시작 후 경과(타임아웃용)
};

/**
 * 타겟 주위를 일정 반경으로 맴돌며(원 그리기) 타겟을 응시 — "간보기".
 * 지정한 (Min~Max) 시간이 지나면 Succeeded. 도중 타겟을 잃으면 Failed.
 */
UCLASS()
class STUDYPROJECT_API UBTTask_StrafeAroundTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_StrafeAroundTarget();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTStrafeMemory); }

protected:
    // 응시·맴돌 대상(Object)
    UPROPERTY(EditAnywhere, Category = "AI")
    struct FBlackboardKeySelector TargetKey;

    // 간보기 중 유지할 거리(이 반경으로 원을 그림)
    UPROPERTY(EditAnywhere, Category = "AI")
    float StrafeRadius = 350.f;

    // 이 거리 안으로 들어오면 간보기/접근 종료 → 공격(BT 다음 노드). StrafeRadius보다 작아야 함.
    UPROPERTY(EditAnywhere, Category = "AI")
    float AttackRange = 170.f;

    // 돌입(접근) 시 이동 입력 강도(0~1). 걷기~뛰기 중간 느낌은 0.6 정도.
    UPROPERTY(EditAnywhere, Category = "AI")
    float ApproachScale = 0.6f;

    // 돌입 후 이 시간 안에 사거리에 못 들면 포기하고 다시 간보기로(놓침 방지)
    UPROPERTY(EditAnywhere, Category = "AI")
    float CommitTimeout = 2.5f;

    // 맴도는 이동 입력 강도(0~1)
    UPROPERTY(EditAnywhere, Category = "AI")
    float MoveScale = 0.7f;

    // 간보는 시간(랜덤 범위)
    UPROPERTY(EditAnywhere, Category = "AI")
    float MinDuration = 1.5f;

    UPROPERTY(EditAnywhere, Category = "AI")
    float MaxDuration = 3.0f;

    // 타겟 응시 회전 속도
    UPROPERTY(EditAnywhere, Category = "AI")
    float TurnSpeed = 8.f;
};
