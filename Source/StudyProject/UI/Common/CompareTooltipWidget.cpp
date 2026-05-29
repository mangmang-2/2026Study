#include "CompareTooltipWidget.h"
#include "Components/CanvasPanelSlot.h"

void UCompareTooltipWidget::SetCompareData(const FItemData& NewItem, const FItemData& EquippedItem)
{
    int32 ATKDiff = NewItem.BaseATK - EquippedItem.BaseATK;
    int32 DEFDiff = NewItem.BaseDEF - EquippedItem.BaseDEF;
    int32 HPDiff  = NewItem.BaseHP  - EquippedItem.BaseHP;
    OnCompareDataSet(NewItem, EquippedItem, ATKDiff, DEFDiff, HPDiff);
}

void UCompareTooltipWidget::SetPosition(FVector2D ScreenPos)
{
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
    if (CanvasSlot) CanvasSlot->SetPosition(ScreenPos);
}
