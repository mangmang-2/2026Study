#include "GA_Death.h"
#include "StudyGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Character/CharacterBase.h"
#include "Character/EnemyCharacter.h"

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
        // 플레이어면 입력 차단(적은 AIController라 no-op)
        if (APlayerController* PC = Cast<APlayerController>(Char->GetController()))
        {
            Char->DisableInput(PC);
        }
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
    // 서버 권위에서 래그돌을 전 클라에 멀티캐스트(물리는 복제 안 되므로).
    // 플레이어(ACharacterBase)와 적(AEnemyCharacter)은 서로 다른 베이스라 각자 멀티캐스트 호출.
    AActor* Avatar = GetAvatarActorFromActorInfo();
    if (ACharacterBase* PlayerChar = Cast<ACharacterBase>(Avatar))
    {
        PlayerChar->Multicast_EnterRagdoll();
    }
    else if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Avatar))
    {
        Enemy->Multicast_EnterRagdoll();
        // 적 시체는 일정 시간 후 소멸(서버에서 SetLifeSpan → 전 클라 동기 파괴)
        Enemy->SetLifeSpan(6.0f);
    }
    // 사망 상태 유지를 위해 EndAbility 호출하지 않음(상태 태그 보존)
}
