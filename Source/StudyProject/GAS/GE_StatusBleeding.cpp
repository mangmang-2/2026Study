#include "GE_StatusBleeding.h"
#include "CombatAttributeSet.h"
#include "StudyGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_StatusBleeding::UGE_StatusBleeding()
{
    // 5초 동안 1초마다 출혈 피해(총 5틱)
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));
    Period.Value = 1.0f;
    bExecutePeriodicEffectOnApplication = true;

    FGameplayModifierInfo Mod;
    Mod.Attribute = UCombatAttributeSet::GetDamageAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(7.f));
    Modifiers.Add(Mod);
}

void UGE_StatusBleeding::PostInitProperties()
{
    Super::PostInitProperties();

    // 생성자에서 컴포넌트 추가 금지 → PostInitProperties에서 Status.Bleeding 부여
    UTargetTagsGameplayEffectComponent& TagComp = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer TagChanges;
    TagChanges.Added.AddTag(StudyTags::Status_Bleeding);
    TagComp.SetAndApplyTargetTagChanges(TagChanges);
}
