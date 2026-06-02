#include "GE_RegenSP.h"
#include "CombatAttributeSet.h"

UGE_RegenSP::UGE_RegenSP()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    Period.Value = 0.5f;

    FGameplayModifierInfo Mod;
    Mod.Attribute = UCombatAttributeSet::GetSPAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.f));
    Modifiers.Add(Mod);
}
