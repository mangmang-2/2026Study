#include "GA_Death.h"
#include "StudyGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

UGA_Death::UGA_Death()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // Event.Death 수신 시 자동 활성화
    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = StudyTags::Event_Death;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::State_Dead);
    SetAssetTags(Tags);
}

void UGA_Death::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    CommitAbility(Handle, ActorInfo, ActivationInfo);

    // 이동/입력 정지
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char)
    {
        if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
        {
            Move->StopMovementImmediately();
            Move->DisableMovement();
        }
    }

    if (DeathMontage == nullptr)
    {
        // 몽타주 없으면 바로 래그돌
        OnDeathMontageFinished();
        return;
    }

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DeathMontage, 1.0f);
    if (MontageTask == nullptr)
    {
        OnDeathMontageFinished();
        return;
    }

    MontageTask->OnCompleted.AddDynamic(this, &UGA_Death::OnDeathMontageFinished);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_Death::OnDeathMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Death::OnDeathMontageFinished);
    MontageTask->ReadyForActivation();
}

void UGA_Death::OnDeathMontageFinished()
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Char == nullptr)
    {
        return;
    }

    // 래그돌 전환
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
    // 사망 상태 유지를 위해 EndAbility 호출하지 않음(상태 태그 보존)
}
