#include "CombatGameplayAbility.h"
#include "AbilitySystemComponent.h"

UCombatGameplayAbility::UCombatGameplayAbility()
{
    // 콤보/회피 등은 인스턴스별 상태가 필요
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 기본은 로컬 예측 활성화(서버 권위 + 클라 예측)
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UCombatGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);

    if (bActivateOnGranted && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
    }
}
