#include "ConfirmPopupWidget.h"
#include "Components/Button.h"

void UConfirmPopupWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (ConfirmBtn)
    {
        ConfirmBtn->OnClicked.AddDynamic(this, &UConfirmPopupWidget::HandleConfirmClicked);
    }
    if (CancelBtn)
    {
        CancelBtn->OnClicked.AddDynamic(this, &UConfirmPopupWidget::HandleCancelClicked);
    }
}

void UConfirmPopupWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (ConfirmBtn)
    {
        ConfirmBtn->OnClicked.RemoveDynamic(this, &UConfirmPopupWidget::HandleConfirmClicked);
    }
    if (CancelBtn)
    {
        CancelBtn->OnClicked.RemoveDynamic(this, &UConfirmPopupWidget::HandleCancelClicked);
    }
}

void UConfirmPopupWidget::SetMessage(const FText& Title, const FText& Body)
{
    OnMessageSet(Title, Body);
}

void UConfirmPopupWidget::HandleConfirmClicked()
{
    OnConfirmed.Broadcast();
    SetVisibility(ESlateVisibility::Collapsed);
}

void UConfirmPopupWidget::HandleCancelClicked()
{
    SetVisibility(ESlateVisibility::Collapsed);
}
