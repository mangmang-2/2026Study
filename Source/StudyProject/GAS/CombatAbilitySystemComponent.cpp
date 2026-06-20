#include "CombatAbilitySystemComponent.h"
#include "CombatGameplayAbility.h"
#include "GA_Combo.h"
#include "StudyGameplayTags.h"

bool UCombatAbilitySystemComponent::TryActivateAbilityByInputTag(FGameplayTag InputTag)
{
    if (InputTag.IsValid() == false)
    {
        return false;
    }

    // 감전(스턴) 중엔 입력으로 어빌리티 발동 불가
    if (HasMatchingGameplayTag(StudyTags::Status_Shocked))
    {
        return false;
    }

    // 이미 활성 중인 콤보면 재활성화 대신 다음 타를 버퍼링한다.
    if (NotifyActiveComboForTag(InputTag))
    {
        // 클라의 NotifyComboInput은 로컬이라 서버 인스턴스가 못 받음 → 서버에도 전달
        if (IsOwnerActorAuthoritative() == false)
        {
            ServerNotifyComboInput(InputTag);
        }
        return true;
    }

    // 활성 콤보가 없으면 새로 발동(LocalPredicted라 GAS가 서버로 복제)
    ABILITYLIST_SCOPE_LOCK();
    for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        const UCombatGameplayAbility* GA = Cast<UCombatGameplayAbility>(Spec.Ability);
        if (GA != nullptr && GA->InputTag.IsValid() && GA->InputTag == InputTag)
        {
            return TryActivateAbility(Spec.Handle);
        }
    }
    return false;
}

bool UCombatAbilitySystemComponent::NotifyActiveComboForTag(FGameplayTag InputTag)
{
    ABILITYLIST_SCOPE_LOCK();
    for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        const UCombatGameplayAbility* GA = Cast<UCombatGameplayAbility>(Spec.Ability);
        if (GA == nullptr || GA->InputTag.IsValid() == false || GA->InputTag != InputTag)
        {
            continue;
        }
        for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
        {
            if (Instance != nullptr && Instance->IsActive())
            {
                if (UGA_Combo* Combo = Cast<UGA_Combo>(Instance))
                {
                    Combo->NotifyComboInput();
                    return true;
                }
            }
        }
    }
    return false;
}

void UCombatAbilitySystemComponent::ServerNotifyComboInput_Implementation(FGameplayTag InputTag)
{
    NotifyActiveComboForTag(InputTag);
}

bool UCombatAbilitySystemComponent::IsMovementLocked() const
{
    for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        if (Spec.IsActive() == false)
        {
            continue;
        }

        const UCombatGameplayAbility* GA = Cast<UCombatGameplayAbility>(Spec.Ability);
        if (GA != nullptr && GA->bLocksMovement)
        {
            return true;
        }
    }
    return false;
}
