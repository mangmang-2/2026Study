#include "HUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UHUDWidget::UpdateHP(float Current, float Max)
{
    if (HPBar)  HPBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
    if (HPText) HPText->SetText(FText::Format(
        FText::FromString(TEXT("{0} / {1}")),
        FMath::RoundToInt(Current), FMath::RoundToInt(Max)));
}

void UHUDWidget::UpdateSP(float Current, float Max)
{
    if (SPBar)  SPBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
    if (SPText) SPText->SetText(FText::Format(
        FText::FromString(TEXT("{0} / {1}")),
        FMath::RoundToInt(Current), FMath::RoundToInt(Max)));
}

void UHUDWidget::UpdateLevel(int32 Level)
{
    if (LevelText) LevelText->SetText(FText::Format(
        FText::FromString(TEXT("Lv.{0}")), Level));
}

void UHUDWidget::FlashHPBar()
{
    PlayHPFlashAnimation();
}

void UHUDWidget::FlashSPBar()
{
    PlaySPFlashAnimation();
}

void UHUDWidget::ShowAcquisitionNotify(UTexture2D* Icon, const FText& ItemName)
{
    // Blueprint로 위임: SlideIn → 3초 대기 → FadeOut
}

void UHUDWidget::UpdateQuestTracker(const FText& InQuestText)
{
    if (QuestText) QuestText->SetText(InQuestText);
}

void UHUDWidget::ShowLockOnHP(const FText& Name, float HP, float Max)
{
    if (LockOnHPBar)    LockOnHPBar->SetPercent(Max > 0.f ? HP / Max : 0.f);
    if (LockOnNameText) LockOnNameText->SetText(Name);
    if (LockOnHPBar)    LockOnHPBar->SetVisibility(ESlateVisibility::Visible);
    if (LockOnNameText) LockOnNameText->SetVisibility(ESlateVisibility::Visible);
}

void UHUDWidget::HideLockOnHP()
{
    if (LockOnHPBar)    LockOnHPBar->SetVisibility(ESlateVisibility::Collapsed);
    if (LockOnNameText) LockOnNameText->SetVisibility(ESlateVisibility::Collapsed);
}

void UHUDWidget::UpdateComboCount(int32 Count)
{
    if (ComboText)
    {
        ComboText->SetText(FText::Format(FText::FromString(TEXT("HIT {0}")), Count));
        ComboText->SetVisibility(Count > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (Count > 0) PlayComboScaleAnimation();
}

void UHUDWidget::HideComboCount()
{
    PlayComboFadeOut();
    if (ComboText) ComboText->SetVisibility(ESlateVisibility::Collapsed);
}

void UHUDWidget::ShowFinisherPrompt()
{
    PlayFinisherPulse();
}

void UHUDWidget::HideFinisherPrompt()
{
}
