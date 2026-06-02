#include "GA_Executed.h"
#include "StudyGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

UGA_Executed::UGA_Executed()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // Event.Executed 수신 시 자동 활성화
    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = StudyTags::Event_Executed;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::State_Dead);
    SetAssetTags(Tags);
}

void UGA_Executed::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    CommitAbility(Handle, ActorInfo, ActivationInfo);

    // 사망 상태로 표시(다른 반응 차단) + 이동 정지
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        if (ASC->HasMatchingGameplayTag(StudyTags::State_Dead) == false)
        {
            ASC->AddLooseGameplayTag(StudyTags::State_Dead);
        }
    }
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char != nullptr)
    {
        if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
        {
            Move->StopMovementImmediately();
            Move->DisableMovement();
        }
    }

    if (ExecutedMontage == nullptr)
    {
        OnMontageFinished();
        return;
    }

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ExecutedMontage, 1.0f);
    if (MontageTask == nullptr)
    {
        OnMontageFinished();
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_Executed::OnMontageFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_Executed::OnMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Executed::OnMontageFinished);
    MontageTask->ReadyForActivation();
}

void UGA_Executed::OnMontageFinished()
{
    // 래그돌 사망
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char == nullptr)
    {
        return;
    }

    if (USkeletalMeshComponent* Mesh = Char->GetMesh())
    {
        Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetAllBodiesSimulatePhysics(true);
        Mesh->SetSimulatePhysics(true);
        Mesh->WakeAllRigidBodies();
    }
    if (UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    // 사망 상태 유지를 위해 EndAbility 호출하지 않음
}
