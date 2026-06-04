#include "GE_StatusChilled.h"
#include "StudyGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_StatusChilled::UGE_StatusChilled()
{
    // 3초 둔화(데미지 없음) — 태그만 부여, 만료 시 자동 제거
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));
}

void UGE_StatusChilled::PostInitProperties()
{
    Super::PostInitProperties();

    // 생성자에서 컴포넌트 추가 금지 → PostInitProperties에서 Status.Chilled 부여
    UTargetTagsGameplayEffectComponent& TagComp = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer TagChanges;
    TagChanges.Added.AddTag(StudyTags::Status_Chilled);
    TagComp.SetAndApplyTargetTagChanges(TagChanges);
}
