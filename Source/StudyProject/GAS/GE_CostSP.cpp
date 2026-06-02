#include "GE_CostSP.h"
#include "CombatAttributeSet.h"

UGE_CostSP::UGE_CostSP()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = UCombatAttributeSet::GetSPAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-20.f));
    Modifiers.Add(Mod);
}
