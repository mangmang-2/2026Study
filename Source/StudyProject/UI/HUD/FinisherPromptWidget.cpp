#include "FinisherPromptWidget.h"

void UFinisherPromptWidget::Show()
{
    SetVisibility(ESlateVisibility::Visible);
    OnShow();
}

void UFinisherPromptWidget::Hide()
{
    SetVisibility(ESlateVisibility::Collapsed);
    OnHide();
}
