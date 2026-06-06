#include "GA_Finisher.h"
#include "ComboData.h"
#include "StudyGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "CollisionShape.h"

UGA_Finisher::UGA_Finisher()
{
    InputTag = StudyTags::Input_Finisher;
    bLocksMovement = true;

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_Finisher);
    SetAssetTags(Tags);
}

void UGA_Finisher::ActivateAbility(
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

    AActor* Avatar = GetAvatarActorFromActorInfo();
    if (Avatar == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FWeaponComboData ComboData;
    UComboLibrary::GetWeaponComboData(Avatar, ComboDataTable, DefaultComboRow, ComboData);
    UAnimMontage* Montage = ComboData.FinisherMontage;
    if (Montage == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 앞에 있는 적(ASC 보유) 탐색
    AActor* Target = nullptr;
    if (UWorld* World = Avatar->GetWorld())
    {
        const FVector Start = Avatar->GetActorLocation();
        const FVector End = Start + Avatar->GetActorForwardVector() * FinisherRange;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(Avatar);

        TArray<FHitResult> Hits;
        World->SweepMultiByChannel(
            Hits, Start, End, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeSphere(FinisherRadius), Params);
        for (const FHitResult& Hit : Hits)
        {
            AActor* A = Hit.GetActor();
            if (A != nullptr && UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(A) != nullptr)
            {
                Target = A;
                break;
            }
        }
    }

    // 대상이 있으면 앞으로 워프 + 마주보게 + 처형 이벤트 전송
    if (Target != nullptr)
    {
        const FVector WarpLoc = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * WarpDistance;
        Target->SetActorLocation(WarpLoc, false, nullptr, ETeleportType::TeleportPhysics);
        const FRotator ToPlayer = (Avatar->GetActorLocation() - WarpLoc).Rotation();
        Target->SetActorRotation(FRotator(0.f, ToPlayer.Yaw, 0.f));

        if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
        {
            FGameplayEventData Payload;
            Payload.EventTag = StudyTags::Event_Executed;
            Payload.Instigator = Avatar;
            Payload.Target = Target;
            TargetASC->HandleGameplayEvent(StudyTags::Event_Executed, &Payload);
        }

        // 처형 연출 시작(슬로모션 + 카메라 전환)
        StartCinematic(Target);
    }

    const float PlayRate = (ComboData.AttackPlayRate > 0.f) ? ComboData.AttackPlayRate : 1.0f;
    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, PlayRate);
    if (MontageTask == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_Finisher::OnMontageFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_Finisher::OnMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Finisher::OnMontageFinished);
    MontageTask->ReadyForActivation();
}

void UGA_Finisher::StartCinematic(AActor* Target)
{
    AActor* Avatar = GetAvatarActorFromActorInfo();
    UWorld* World = (Avatar != nullptr) ? Avatar->GetWorld() : nullptr;
    if (Avatar == nullptr || Target == nullptr || World == nullptr)
    {
        return;
    }

    // 글로벌 타임딜레이션은 멀티에서 모두를 늦추므로 미사용 — 카메라 연출만 유지

    // 연출 카메라 — 두 캐릭터 측면에서 잡기(BP 파라미터로 조절)
    const FVector PlayerLoc = Avatar->GetActorLocation();
    const FVector TargetLoc = Target->GetActorLocation();
    const FVector MidGround = (PlayerLoc + TargetLoc) * 0.5f;
    FVector ToTarget = TargetLoc - PlayerLoc;
    ToTarget.Z = 0.f;
    ToTarget = ToTarget.GetSafeNormal();
    const FVector Side = FVector::CrossProduct(ToTarget, FVector::UpVector).GetSafeNormal();

    const FVector LookAt = MidGround + FVector(0.f, 0.f, CameraLookAtHeight);
    const FVector CamLoc = MidGround
        + Side * CameraSideDistance
        - ToTarget * CameraBackOffset
        + FVector(0.f, 0.f, CameraHeight);
    const FRotator CamRot = (LookAt - CamLoc).Rotation();

    ACameraActor* Cam = World->SpawnActor<ACameraActor>(CamLoc, CamRot);
    if (Cam != nullptr)
    {
        CineCamera = Cam;
        if (UCameraComponent* CamComp = Cam->GetCameraComponent())
        {
            CamComp->SetFieldOfView(CameraFOV);
        }
        if (APawn* AvatarPawn = Cast<APawn>(Avatar))
        {
            if (APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController()))
            {
                PC->SetViewTargetWithBlend(Cam, CameraBlendTime);
            }
        }
    }
}

void UGA_Finisher::EndCinematic()
{
    AActor* Avatar = GetAvatarActorFromActorInfo();
    // 글로벌 타임딜레이션 미사용(멀티 호환) — 복원 불필요.

    // 카메라 복귀
    if (APawn* AvatarPawn = Cast<APawn>(Avatar))
    {
        if (APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController()))
        {
            PC->SetViewTargetWithBlend(AvatarPawn, CameraBlendTime);
        }
    }
    if (CineCamera != nullptr)
    {
        CineCamera->Destroy();
        CineCamera = nullptr;
    }
}

void UGA_Finisher::OnMontageFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Finisher::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    EndCinematic();   // 슬로모션/카메라 항상 복구
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
