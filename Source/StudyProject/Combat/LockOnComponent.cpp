#include "LockOnComponent.h"
#include "Character/EnemyCharacter.h"
#include "GAS/StudyGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ULockOnComponent::ULockOnComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void ULockOnComponent::ToggleLockOn()
{
    if (CurrentTarget.IsValid())
    {
        StopLockOn();
        return;
    }

    AActor* Target = FindBestTarget(0.f);
    if (Target != nullptr)
    {
        StartLockOn(Target);
    }
}

void ULockOnComponent::SwitchTarget(float Direction)
{
    if (CurrentTarget.IsValid() == false)
    {
        return;
    }
    AActor* Next = FindBestTarget(Direction);
    if (Next != nullptr && Next != CurrentTarget.Get())
    {
        CurrentTarget = Next;
    }
}

bool ULockOnComponent::IsValidTarget(AActor* Target) const
{
    if (Target == nullptr)
    {
        return false;
    }
    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return false;
    }
    if (FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation()) > LoseRange)
    {
        return false;
    }
    // 사망한 적 제외
    if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
    {
        if (ASC->HasMatchingGameplayTag(StudyTags::State_Dead))
        {
            return false;
        }
    }
    return true;
}

AActor* ULockOnComponent::FindBestTarget(float SideSign) const
{
    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (Owner == nullptr)
    {
        return nullptr;
    }
    AController* Ctrl = Owner->GetController();
    if (Ctrl == nullptr)
    {
        return nullptr;
    }

    const FVector OwnerLoc = Owner->GetActorLocation();
    const FVector CamFwd = Ctrl->GetControlRotation().Vector();
    const FVector CamRight = FRotationMatrix(Ctrl->GetControlRotation()).GetUnitAxis(EAxis::Y);

    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacter::StaticClass(), Enemies);

    AActor* Best = nullptr;
    float BestScore = -2.f;   // dot 점수 범위 [-1,1]
    for (AActor* Enemy : Enemies)
    {
        if (IsValidTarget(Enemy) == false || Enemy == CurrentTarget.Get())
        {
            continue;
        }
        const FVector ToEnemy = Enemy->GetActorLocation() - OwnerLoc;
        if (ToEnemy.SizeSquared() > SearchRange * SearchRange)
        {
            continue;
        }
        const FVector Dir = ToEnemy.GetSafeNormal();
        const float ViewDot = FVector::DotProduct(CamFwd, Dir);
        if (ViewDot < MinViewDot)
        {
            continue;
        }

        float Score = ViewDot;   // 기본: 가장 정면에 가까운 적
        if (SideSign != 0.f)
        {
            // 좌우 전환: 지정한 쪽에 있는 적만, 그 중 가장 정면
            const float RightDot = FVector::DotProduct(CamRight, Dir);
            if (FMath::Sign(RightDot) != FMath::Sign(SideSign))
            {
                continue;
            }
            Score = ViewDot;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            Best = Enemy;
        }
    }
    return Best;
}

void ULockOnComponent::StartLockOn(AActor* Target)
{
    CurrentTarget = Target;

    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (Owner != nullptr)
    {
        Owner->bUseControllerRotationYaw = true;
        if (UCharacterMovementComponent* Move = Owner->GetCharacterMovement())
        {
            Move->bOrientRotationToMovement = false;
        }
    }
}

void ULockOnComponent::StopLockOn()
{
    CurrentTarget = nullptr;

    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (Owner != nullptr)
    {
        Owner->bUseControllerRotationYaw = false;
        if (UCharacterMovementComponent* Move = Owner->GetCharacterMovement())
        {
            Move->bOrientRotationToMovement = true;
        }
    }
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentTarget.IsValid() == false)
    {
        return;
    }

    if (IsValidTarget(CurrentTarget.Get()) == false)
    {
        StopLockOn();
        return;
    }

    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (Owner == nullptr)
    {
        return;
    }
    AController* Ctrl = Owner->GetController();
    if (Ctrl == nullptr)
    {
        return;
    }

    // 타겟(가슴 높이)을 향해 컨트롤 회전 보간 → 카메라/캐릭터가 타겟을 바라봄
    const FVector AimAt = CurrentTarget->GetActorLocation() + FVector(0.f, 0.f, 40.f);
    const FVector ToTarget = AimAt - Owner->GetActorLocation();
    const FRotator Desired = ToTarget.Rotation();
    const FRotator Clamped(FMath::Clamp(Desired.Pitch, -40.f, 10.f), Desired.Yaw, 0.f);
    const FRotator NewRot = FMath::RInterpTo(Ctrl->GetControlRotation(), Clamped, DeltaTime, RotationInterpSpeed);
    Ctrl->SetControlRotation(NewRot);
}
