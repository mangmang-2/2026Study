#include "EnemyHPBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UEnemyHPBarWidget::UpdateHP(float Current, float Max)
{
    if (HPBar) HPBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
    if (HPText) HPText->SetText(FText::Format(
        FText::FromString(TEXT("{0} / {1}")),
        FMath::RoundToInt(Current), FMath::RoundToInt(Max)));

    SetVisibility(ESlateVisibility::Visible);
}

void UEnemyHPBarWidget::ShowForDuration(float Seconds)
{
    SetVisibility(ESlateVisibility::Visible);
    PlayFadeOutAfterDelay(Seconds);
}
