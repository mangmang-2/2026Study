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

    // 사망 시 정지
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
    if (ASC != nullptr && ASC->HasMatchingGameplayTag(StudyTags::State_Dead))
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
        // 접근(추격 중엔 콤보 카운트 리셋)
        Enemy->AddMovementInput(Dir, 1.f);
        ComboCount = 0;
    }
    else
    {
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
