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

    // 행동 불가 상태(사망/공중·넉다운/경직/피격)면 AI 완전 정지 — 따라다니거나 바라보지도 않음
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
    if (ASC != nullptr &&
        (ASC->HasMatchingGameplayTag(StudyTags::State_Dead)
         || ASC->HasMatchingGameplayTag(StudyTags::State_AirBorne)
         || ASC->HasMatchingGameplayTag(StudyTags::Status_Staggered)
         || ASC->HasMatchingGameplayTag(StudyTags::Status_Shocked)
         || ASC->HasMatchingGameplayTag(StudyTags::State_Attacking)
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
        // 돌진은 같은 높이(층)에서만 — 높이차가 크면(다른 층) 수평 돌진하다 벽에 박으므로 금지
        const bool bChargeHeightOK = (FMath::Abs(VertDiff) <= ChargeMaxHeightDiff);

        // 벽이 가로막으면 돌진 금지(벽에 박고 계속 대쉬만 쓰는 루프 방지) — 보스→플레이어 직선에
        // World 정적 장애물이 있으면 막힌 것으로 보고, 돌진 대신 navmesh로 우회 접근하게 한다.
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

        // 돌진 준비(쿨다운) 중엔 걸어서 접근하지 않고 제자리 대기 — "앞까지 와서 친다" 방지
        if (bInChargeBand)
        {
            StopMovement();
            ComboCount = 0;
            return;
        }

        // 접근: navmesh 길찾기로 추격(계단 등반/벽 우회 → 다른 층 플레이어도 따라감).
        // 이미 이동 중이면 재요청 안 함(목표 액터 추적). navmesh 없거나 경로 없으면 직선 이동 폴백.
        if (GetMoveStatus() != EPathFollowingStatus::Moving)
        {
            // 공격 사거리 안쪽까지 확실히 접근(정지 거리 = 공격 사거리의 절반) — "멀리서 멈춰 안 때림" 방지
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

        // 플레이어가 공중에 높이 떠 있으면(머리 위) 공격하지 않음 — 허공질 방지
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
