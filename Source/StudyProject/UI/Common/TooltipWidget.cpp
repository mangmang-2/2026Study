#include "TooltipWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UTooltipWidget::SetItemData(const FItemData& Data)
{
    OnItemDataSet(Data);
}

void UTooltipWidget::SetPosition(FVector2D ScreenPos)
{
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
    if (CanvasSlot) CanvasSlot->SetPosition(ScreenPos);
}
