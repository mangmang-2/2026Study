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

void UInventoryGridWidget::RefreshGrid(const TArray<FInventorySlot>& Slots, const TArray<int32>& SourceIndices)
{
    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;

    const bool bMapped = (SourceIndices.Num() > 0);   // 매핑 제공 = 필터/압축 표시 중

    for (int32 i = 0; i < SlotWidgets.Num(); ++i)
    {
        UItemSlotWidget* SlotW = SlotWidgets[i];
        if (!SlotW) continue;

        // 그리드 칸 → 실제 인벤 슬롯 인덱스(매핑 밖이면 -1, 비매핑이면 위치=인덱스)
        const int32 RealIndex = bMapped ? (SourceIndices.IsValidIndex(i) ? SourceIndices[i] : -1) : i;
        SlotW->SlotIndex = RealIndex;

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
