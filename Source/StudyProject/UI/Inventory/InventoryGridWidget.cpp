#include "InventoryGridWidget.h"
#include "UI/Common/ItemSlotWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"

void UInventoryGridWidget::InitGrid(int32 SlotCount)
{
    if (!GridPanel || !SlotWidgetClass) return;

    GridPanel->ClearChildren();
    SlotWidgets.Reset();

    for (int32 i = 0; i < SlotCount; ++i)
    {
        UItemSlotWidget* SlotW = CreateWidget<UItemSlotWidget>(this, SlotWidgetClass);
        if (!SlotW) continue;

        SlotW->SlotIndex   = i;
        SlotW->SlotContext = ESlotContext::Inventory;

        if (UUniformGridSlot* GridSlot = GridPanel->AddChildToUniformGrid(SlotW, i / ColumnCount, i % ColumnCount))
        {
            GridSlot->SetHorizontalAlignment(HAlign_Fill);
            GridSlot->SetVerticalAlignment(VAlign_Fill);
        }
        SlotWidgets.Add(SlotW);
    }
}

void UInventoryGridWidget::RefreshGrid(const TArray<FInventorySlot>& Slots)
{
    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;

    for (int32 i = 0; i < SlotWidgets.Num(); ++i)
    {
        UItemSlotWidget* SlotW = SlotWidgets[i];
        if (!SlotW) continue;

        if (Slots.IsValidIndex(i) && !Slots[i].IsEmpty())
        {
            const FItemData* Data = ItemSub ? ItemSub->GetItemData(Slots[i].ItemID) : nullptr;
            if (Data)
                SlotW->SetItemData(*Data, Slots[i].Quantity, Slots[i].EnhanceLevel);
            else
                SlotW->ClearSlot();
        }
        else
        {
            SlotW->ClearSlot();
        }
    }
}

UItemSlotWidget* UInventoryGridWidget::GetSlotWidget(int32 Index) const
{
    return SlotWidgets.IsValidIndex(Index) ? SlotWidgets[Index] : nullptr;
}
