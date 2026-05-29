#include "InventoryWidget.h"
#include "UI/Inventory/InventoryGridWidget.h"
#include "UI/Common/ItemSlotWidget.h"
#include "UI/Common/TooltipWidget.h"
#include "UI/Common/ContextMenuWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryActionHelper.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UInventoryWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (BoundInventory.IsValid())
    {
        BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
    }
}

void UInventoryWidget::BindToInventory(UInventoryComponent* InvComp)
{
    if (BoundInventory.IsValid())
    {
        BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
    }

    BoundInventory = InvComp;

    if (BoundInventory.IsValid())
    {
        BoundInventory->OnInventoryChanged.AddDynamic(this, &UInventoryWidget::HandleInventoryChanged);
        if (GridWidget && GridWidget->GetSlotWidget(0) == nullptr)
        {
            GridWidget->InitGrid(BoundInventory->GetMaxSlots());
        }
        RefreshInventory();
    }
}

void UInventoryWidget::RefreshInventory()
{
    if (BoundInventory.IsValid() == false || GridWidget == nullptr)
    {
        return;
    }

    TArray<FInventorySlot> Slots;
    for (int32 i = 0; i < BoundInventory->GetMaxSlots(); ++i)
    {
        Slots.Add(BoundInventory->GetSlot(i));
    }

    GridWidget->RefreshGrid(Slots);

    for (int32 i = 0; i < BoundInventory->GetMaxSlots(); ++i)
    {
        UItemSlotWidget* SlotW = GridWidget->GetSlotWidget(i);
        if (SlotW == nullptr)
        {
            continue;
        }

        SlotW->OnSlotHovered.Clear();
        SlotW->OnSlotRightClicked.Clear();
        SlotW->OnSlotDrop.Clear();

        SlotW->OnSlotHovered.AddDynamic(this, &UInventoryWidget::HandleSlotHovered);
        SlotW->OnSlotRightClicked.AddDynamic(this, &UInventoryWidget::HandleSlotRightClicked);
        SlotW->OnSlotDrop.AddDynamic(this, &UInventoryWidget::HandleSlotDrop);
    }
}

void UInventoryWidget::FilterByType(EItemType Type)
{
    CurrentFilter = Type;
    RefreshInventory();
}

void UInventoryWidget::SortItems(ESortMode Mode)
{
    if (BoundInventory.IsValid() == false)
    {
        return;
    }
    CurrentSort = Mode;
    if (Mode == ESortMode::ByRarity)
    {
        BoundInventory->SortByRarity();
    }
    else
    {
        BoundInventory->SortByName();
    }
}

void UInventoryWidget::ShowTooltip(int32 SlotIndex)
{
    if (Tooltip == nullptr || BoundInventory.IsValid() == false)
    {
        return;
    }

    const FInventorySlot& InvSlot = BoundInventory->GetSlot(SlotIndex);
    if (InvSlot.IsEmpty())
    {
        HideTooltip();
        return;
    }

    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;
    const FItemData* Data = ItemSub ? ItemSub->GetItemData(InvSlot.ItemID) : nullptr;
    if (Data == nullptr)
    {
        return;
    }

    Tooltip->SetItemData(*Data);
    Tooltip->SetVisibility(ESlateVisibility::Visible);
}

void UInventoryWidget::HideTooltip()
{
    if (Tooltip != nullptr)
    {
        Tooltip->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UInventoryWidget::ShowContextMenu(int32 SlotIndex)
{
    if (ContextMenu == nullptr || BoundInventory.IsValid() == false)
    {
        return;
    }

    const FInventorySlot& InvSlot = BoundInventory->GetSlot(SlotIndex);
    if (InvSlot.IsEmpty())
    {
        return;
    }

    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;
    const FItemData* Data = ItemSub ? ItemSub->GetItemData(InvSlot.ItemID) : nullptr;
    if (Data == nullptr)
    {
        return;
    }

    ContextMenu->ShowForItem(SlotIndex, *Data, FVector2D::ZeroVector);
}

void UInventoryWidget::HandleSlotHovered(int32 SlotIndex)
{
    if (SlotIndex < 0)
    {
        HideTooltip();
        return;
    }
    ShowTooltip(SlotIndex);
}

void UInventoryWidget::HandleSlotRightClicked(int32 SlotIndex)
{
    ShowContextMenu(SlotIndex);
}

void UInventoryWidget::HandleSlotDrop(int32 FromSlot, int32 ToSlot)
{
    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (Char == nullptr)
    {
        return;
    }
    UInventoryActionHelper::HandleDrop(Context, Context, FromSlot, ToSlot, Char);
}

void UInventoryWidget::HandleInventoryChanged()
{
    RefreshInventory();
}
