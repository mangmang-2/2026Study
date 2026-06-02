#include "GA_AirCombo.h"
#include "StudyGameplayTags.h"

UGA_AirCombo::UGA_AirCombo()
{
    InputTag = StudyTags::Input_AirAttack;

    // 공중 콤보 적중 시 적을 살짝만 다시 띄워(저글) 중력 낮게 유지 → 적의 GA_AirLaunch 재발동.
    // 값이 크면 칠 때마다 적이 위로 솟으니 작게(플레이어 체공과 비슷한 높이로 같이 떠 있게).
    // 주의: 0이면 GA_AirLaunch가 기본값(700)으로 폴백하므로 작은 양수 사용.
    HitEventTag = StudyTags::Event_Launched;
    HitEventMagnitude = 50.f;

    // 플레이어 자신도 공중 타격 성공 시 체공(콤보 도중 빨리 떨어지는 문제 해결)
    bFloatSelfOnHit = true;

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_AirCombo);
    SetAssetTags(Tags);
}

const TArray<TObjectPtr<UAnimMontage>>& UGA_AirCombo::SelectCombo(const FWeaponComboData& Data) const
{
    return Data.AirCombo;
}
