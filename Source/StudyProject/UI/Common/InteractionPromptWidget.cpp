#include "InteractionPromptWidget.h"
#include "Components/TextBlock.h"

void UInteractionPromptWidget::SetPromptText(const FText& Text)
{
    if (PromptText)
        PromptText->SetText(Text);
}

void UInteractionPromptWidget::Show()
{
    SetVisibility(ESlateVisibility::Visible);
}

void UInteractionPromptWidget::Hide()
{
    SetVisibility(ESlateVisibility::Collapsed);
}
