#include "GA_Launcher.h"
#include "ComboData.h"
#include "StudyGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UGA_Launcher::UGA_Launcher()
{
    InputTag = StudyTags::Input_Launcher;
    bLocksMovement = true;

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_AirLauncher);
    SetAssetTags(Tags);
}

void UGA_Launcher::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FWeaponComboData ComboData;
    if (UComboLibrary::GetWeaponComboData(GetAvatarActorFromActorInfo(), ComboDataTable, DefaultComboRow, ComboData) == false
        || ComboData.LauncherMontage == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    CurrentDamage = ComboData.DamagePerHit;
    CurrentHitFeel = ComboData.HitFeel;
    CurrentLaunchEnemyZ = ComboData.LaunchEnemyZ;
    CurrentLaunchSelfZ = ComboData.LaunchSelfZ;
    CurrentPlayRate = (ComboData.AttackPlayRate > 0.f) ? ComboData.AttackPlayRate : 1.0f;

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ComboData.LauncherMontage, CurrentPlayRate);
    if (MontageTask == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_Launcher::OnMontageFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_Launcher::OnMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Launcher::OnMontageFinished);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_Launcher::OnMontageFinished);
    MontageTask->ReadyForActivation();

    const float ScaledHitDelay = (CurrentPlayRate > 0.f) ? (HitDelay / CurrentPlayRate) : HitDelay;
    UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, ScaledHitDelay);
    if (DelayTask != nullptr)
    {
        DelayTask->OnFinish.AddDynamic(this, &UGA_Launcher::DoMeleeTrace);
        DelayTask->ReadyForActivation();
    }
}

void UGA_Launcher::DoMeleeTrace()
{
    const bool bHit = ApplyMeleeDamage(CurrentDamage, StudyTags::Event_Launched, CurrentLaunchEnemyZ, CurrentHitFeel);

    if (bHit)
    {
        // 적중하면 플레이어도 살짝 떠서 공중 콤보로 연계
        if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
        {
            ApplyLaunchGravity();
            Char->LandedDelegate.AddDynamic(this, &UGA_Launcher::OnSelfLanded);
            Char->LaunchCharacter(FVector(0.f, 0.f, CurrentLaunchSelfZ), false, true);
        }
    }
}

void UGA_Launcher::OnMontageFinished()
{
    // 몽타주가 끝나도 아직 공중일 수 있어 중력 복원은 착지에서만. 여기선 어빌만 종료.
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Launcher::OnSelfLanded(const FHitResult& Hit)
{
    if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        Char->LandedDelegate.RemoveDynamic(this, &UGA_Launcher::OnSelfLanded);
    }
    RestoreGravity();
}

void UGA_Launcher::ApplyLaunchGravity()
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char == nullptr)
    {
        return;
    }
    UCharacterMovementComponent* Move = Char->GetCharacterMovement();
    if (Move == nullptr)
    {
        return;
    }

    if (bGravityActive == false)
    {
        SavedGravityScale = Move->GravityScale;
        bGravityActive = true;
    }
    Move->GravityScale = LaunchGravityScale;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ApexTimerHandle, this, &UGA_Launcher::CheckApex, 0.016f, true);
    }
}

void UGA_Launcher::CheckApex()
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    UCharacterMovementComponent* Move = Char ? Char->GetCharacterMovement() : nullptr;
    if (Move == nullptr)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(ApexTimerHandle);
        }
        return;
    }

    if (Move->Velocity.Z <= 0.f)
    {
        Move->GravityScale = HangGravityScale;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(ApexTimerHandle);
        }
    }
}

void UGA_Launcher::RestoreGravity()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ApexTimerHandle);
    }
    if (bGravityActive)
    {
        if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
        {
            if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
            {
                Move->GravityScale = SavedGravityScale;
            }
        }
        bGravityActive = false;
    }
}
