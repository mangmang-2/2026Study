#include "GE_StatusBurning.h"
#include "CombatAttributeSet.h"
#include "StudyGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_StatusBurning::UGE_StatusBurning()
{
    // 4초 동안 0.5초마다 화염 피해(총 8틱)
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(4.0f));
    Period.Value = 0.5f;
    bExecutePeriodicEffectOnApplication = true;

    // 주기마다 Damage meta attribute에 가산 → AttributeSet가 HP로 환산
    FGameplayModifierInfo Mod;
    Mod.Attribute = UCombatAttributeSet::GetDamageAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.f));
    Modifiers.Add(Mod);
}

void UGE_StatusBurning::PostInitProperties()
{
    Super::PostInitProperties();

    // 생성자에서 컴포넌트(이름 없는 NewObject) 추가는 CDO 생성 중 금지 → PostInitProperties에서 부여.
    // 적용 동안 Status.Burning 부여(GE 만료 시 자동 제거).
    UTargetTagsGameplayEffectComponent& TagComp = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer TagChanges;
    TagChanges.Added.AddTag(StudyTags::Status_Burning);
    TagComp.SetAndApplyTargetTagChanges(TagChanges);
}
