#include "ContextMenuWidget.h"

void UContextMenuWidget::ShowForItem(int32 InSlotIndex, const FItemData& Data, FVector2D ScreenPos)
{
    TargetSlotIndex = InSlotIndex;
    SetVisibility(ESlateVisibility::Visible);
    OnShow(InSlotIndex, Data, ScreenPos);
}

void UContextMenuWidget::Close()
{
    TargetSlotIndex = -1;
    SetVisibility(ESlateVisibility::Collapsed);
}
