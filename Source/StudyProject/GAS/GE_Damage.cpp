#include "GE_Damage.h"
#include "CombatAttributeSet.h"
#include "StudyGameplayTags.h"

UGE_Damage::UGE_Damage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = UCombatAttributeSet::GetDamageAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;

    FSetByCallerFloat SetByCaller;
    SetByCaller.DataTag = StudyTags::Data_Damage;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

    Modifiers.Add(Mod);
}
