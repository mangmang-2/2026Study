#include "GE_StatusShocked.h"
#include "StudyGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_StatusShocked::UGE_StatusShocked()
{
    // 1.5초 스턴(데미지 없음) — 태그만 부여, 만료 시 자동 제거
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.5f));
}

void UGE_StatusShocked::PostInitProperties()
{
    Super::PostInitProperties();

    // 생성자에서 컴포넌트 추가 금지 → PostInitProperties에서 Status.Shocked 부여
    UTargetTagsGameplayEffectComponent& TagComp = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer TagChanges;
    TagChanges.Added.AddTag(StudyTags::Status_Shocked);
    TagComp.SetAndApplyTargetTagChanges(TagChanges);
}
