#include "ComboCounterWidget.h"

void UComboCounterWidget::SetCount(int32 Count)
{
    OnCountSet(Count);
    PlayPulseScale();
    SetVisibility(Count > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UComboCounterWidget::ResetCount()
{
    PlayFadeOut();
    SetVisibility(ESlateVisibility::Collapsed);
}
