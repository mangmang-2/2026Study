#include "GA_AirCombo.h"
#include "StudyGameplayTags.h"

UGA_AirCombo::UGA_AirCombo()
{
    InputTag = StudyTags::Input_AirAttack;

    // 적중마다 적을 살짝 저글(작게 — 0이면 GA_AirLaunch가 700으로 폴백)
    HitEventTag = StudyTags::Event_Launched;
    HitEventMagnitude = 50.f;

    // 마지막 타는 슬램(매그니튜드 0이면 적 GA의 기본 SlamDownSpeed)
    LastHitEventTag = StudyTags::Event_Slammed;
    LastHitEventMagnitude = 0.f;

    // 콤보 도중 안 떨어지게 자신도 체공
    bFloatSelfOnHit = true;

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_AirCombo);
    SetAssetTags(Tags);
}

const TArray<TObjectPtr<UAnimMontage>>& UGA_AirCombo::SelectCombo(const FWeaponComboData& Data) const
{
    return Data.AirCombo;
}
