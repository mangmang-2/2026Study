#include "CombatAbilitySystemComponent.h"
#include "CombatGameplayAbility.h"

bool UCombatAbilitySystemComponent::TryActivateAbilityByInputTag(FGameplayTag InputTag)
{
    if (!InputTag.IsValid())
    {
        return false;
    }

    ABILITYLIST_SCOPE_LOCK();
    for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        const UCombatGameplayAbility* GA = Cast<UCombatGameplayAbility>(Spec.Ability);
        if (GA && GA->InputTag.IsValid() && GA->InputTag == InputTag)
        {
            return TryActivateAbility(Spec.Handle);
        }
    }
    return false;
}
