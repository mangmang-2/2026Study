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
#include "TimerManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UInventoryWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TooltipTimerHandle);
    }
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

    // 커서 근처에 위치(뷰포트 스케일 보정), 살짝 우하단 오프셋
    const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
    FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
    Tooltip->SetPosition(MousePos + FVector2D(16.f, 16.f) / FMath::Max(Scale, 0.01f));

    Tooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
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
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(TooltipTimerHandle);
    }

    if (SlotIndex < 0)
    {
        PendingTooltipSlot = -1;
        HideTooltip();
        return;
    }

    // 같은 슬롯에 TooltipDelay초 머무르면 표시
    PendingTooltipSlot = SlotIndex;
    if (World)
    {
        World->GetTimerManager().SetTimer(
            TooltipTimerHandle, this, &UInventoryWidget::ShowPendingTooltip, TooltipDelay, false);
    }
}

void UInventoryWidget::ShowPendingTooltip()
{
    if (PendingTooltipSlot >= 0)
    {
        ShowTooltip(PendingTooltipSlot);
    }
}

void UInventoryWidget::HandleSlotRightClicked(int32 SlotIndex)
{
    ShowContextMenu(SlotIndex);
}

void UInventoryWidget::HandleSlotDrop(ESlotContext SourceContext, int32 FromSlot, int32 ToSlot)
{
    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (Char == nullptr)
    {
        return;
    }
    // 드롭 대상이 인벤이므로 To=Inventory, From=드래그 시작 컨텍스트
    UInventoryActionHelper::HandleDrop(SourceContext, Context, FromSlot, ToSlot, Char);
}

void UInventoryWidget::HandleInventoryChanged()
{
    RefreshInventory();
}
