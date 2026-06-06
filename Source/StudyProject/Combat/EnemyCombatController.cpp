#include "EnemyCombatController.h"
#include "GA_EnemyAttack.h"
#include "StudyGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Navigation/PathFollowingComponent.h"

AEnemyCombatController::AEnemyCombatController()
{
    PrimaryActorTick.bCanEverTick = true;
    AttackAbility = UGA_EnemyAttack::StaticClass();
}

void AEnemyCombatController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ACharacter* Enemy = Cast<ACharacter>(GetPawn());
    if (Enemy == nullptr)
    {
        return;
    }

    // 행동 불가 상태면 AI 완전 정지
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
    if (ASC != nullptr &&
        (ASC->HasMatchingGameplayTag(StudyTags::State_Dead)
         || ASC->HasMatchingGameplayTag(StudyTags::State_AirBorne)
         || ASC->HasMatchingGameplayTag(StudyTags::Status_Staggered)
         || ASC->HasMatchingGameplayTag(StudyTags::Status_Shocked)
         || ASC->HasMatchingGameplayTag(StudyTags::State_Attacking)
         || ASC->HasMatchingGameplayTag(StudyTags::State_Knockdown)
         || ASC->HasMatchingGameplayTag(StudyTags::State_HitReact)))
    {
        return;
    }

    APawn* Target = UGameplayStatics::GetPlayerPawn(this, 0);
    if (Target == nullptr)
    {
        return;
    }

    const FVector RawTo = Target->GetActorLocation() - Enemy->GetActorLocation();
    const float VertDiff = RawTo.Z;   // 양수면 플레이어가 위(공중)
    FVector ToTarget = RawTo;
    ToTarget.Z = 0.f;
    const float Dist = ToTarget.Size();
    if (Dist > AcquireRange)
    {
        return;   // 너무 멀면 대기
    }

    const FVector Dir = (Dist > KINDA_SMALL_NUMBER) ? (ToTarget / Dist) : Enemy->GetActorForwardVector();

    // 항상 플레이어를 바라보게(부드럽게)
    const FRotator TargetRot(0.f, Dir.Rotation().Yaw, 0.f);
    Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaSeconds, TurnSpeed));

    // 몽타주 재생 중(공격/피격/넉다운)에는 이동/공격 안 함
    bool bBusy = false;
    if (USkeletalMeshComponent* Mesh = Enemy->GetMesh())
    {
        if (UAnimInstance* Anim = Mesh->GetAnimInstance())
        {
            bBusy = Anim->IsAnyMontagePlaying();
        }
    }
    if (bBusy)
    {
        return;
    }

    if (Dist > AttackRange)
    {
        const float Now = GetWorld()->GetTimeSeconds();
        // 돌진은 같은 높이에서만(다른 층이면 벽에 박음)
        const bool bChargeHeightOK = (FMath::Abs(VertDiff) <= ChargeMaxHeightDiff);

        // 직선에 벽 있으면 돌진 금지(박는 루프 방지) → navmesh 우회
        bool bClearChargePath = true;
        {
            FHitResult WallHit;
            FCollisionQueryParams QP;
            QP.AddIgnoredActor(Enemy);
            QP.AddIgnoredActor(Target);
            if (GetWorld()->LineTraceSingleByChannel(WallHit, Enemy->GetActorLocation(), Target->GetActorLocation(), ECC_WorldStatic, QP))
            {
                bClearChargePath = false;
            }
        }

        const bool bInChargeBand = (ChargeAbility != nullptr && bChargeHeightOK && bClearChargePath
            && Dist >= ChargeMinRange && Dist <= ChargeMaxRange);

        // 갭클로저: 돌진 사거리 + 같은 높이 + 쿨다운 회복 → 돌진(카이팅 압박)
        if (bInChargeBand && Now - LastChargeTime >= ChargeCooldown && ASC != nullptr)
        {
            StopMovement();
            if (ASC->TryActivateAbilityByClass(ChargeAbility))
            {
                LastChargeTime = Now;
                ComboCount = 0;
                return;
            }
        }

        // 접근: navmesh 추격(계단/우회). 이미 이동 중이면 재요청 안 함. 경로 없으면 직선 폴백.
        if (GetMoveStatus() != EPathFollowingStatus::Moving)
        {
            // 정지 거리 = 사거리 절반(멀리서 멈춰 안 때리는 것 방지)
            const float Accept = FMath::Max(50.f, AttackRange * 0.5f);
            const EPathFollowingRequestResult::Type MoveRes = MoveToActor(Target, Accept);
            if (MoveRes == EPathFollowingRequestResult::Failed)
            {
                Enemy->AddMovementInput(Dir, 1.f);
            }
        }
        ComboCount = 0;
    }
    else
    {
        StopMovement();   // 사거리 안 — 이동 멈추고 공격

        // 플레이어가 머리 위 공중이면 공격 안 함(허공질 방지)
        if (VertDiff > MaxAttackHeight)
        {
            ComboCount = 0;
            return;
        }

        // 사거리 안 — 연속공격(ComboMax회) 후 AttackCooldown 회복
        const float Now = GetWorld()->GetTimeSeconds();
        const float ReqGap = (ComboCount >= ComboMax) ? AttackCooldown : ComboGap;
        if (Now - LastAttackTime >= ReqGap && ASC != nullptr)
        {
            if (ComboCount >= ComboMax)
            {
                ComboCount = 0;   // 회복 끝 → 새 콤보 시작
            }
            TSubclassOf<UGameplayAbility> Chosen = AttackAbility;
            if (AttackAbilities.Num() > 0)
            {
                Chosen = AttackAbilities[FMath::RandRange(0, AttackAbilities.Num() - 1)];
            }
            if (Chosen != nullptr && ASC->TryActivateAbilityByClass(Chosen))
            {
                LastAttackTime = Now;
                ++ComboCount;
            }
        }
    }
}
