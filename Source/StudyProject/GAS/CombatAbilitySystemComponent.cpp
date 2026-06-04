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

    ABILITYLIST_SCOPE_LOCK();
    for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        const UCombatGameplayAbility* GA = Cast<UCombatGameplayAbility>(Spec.Ability);
        if (GA != nullptr && GA->InputTag.IsValid() && GA->InputTag == InputTag)
        {
            // 이미 활성 중인 콤보 어빌리티면 재활성화 대신 다음 타를 버퍼링한다.
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
            return TryActivateAbility(Spec.Handle);
        }
    }
    return false;
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
