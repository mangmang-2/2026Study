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
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Framework/Application/SlateApplication.h"

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (FilterAllButton)        FilterAllButton->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::OnFilterAllClicked);
    if (FilterWeaponButton)     FilterWeaponButton->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::OnFilterWeaponClicked);
    if (FilterArmorButton)      FilterArmorButton->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::OnFilterArmorClicked);
    if (FilterConsumableButton) FilterConsumableButton->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::OnFilterConsumableClicked);
    if (SortRarityButton)       SortRarityButton->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::OnSortRarityClicked);
    if (SortNameButton)         SortNameButton->OnClicked.AddUniqueDynamic(this, &UInventoryWidget::OnSortNameClicked);
}

void UInventoryWidget::OnFilterAllClicked()        { FilterByType(EItemType::All); }
void UInventoryWidget::OnFilterWeaponClicked()     { FilterByType(EItemType::Weapon); }
void UInventoryWidget::OnFilterArmorClicked()      { FilterByType(EItemType::Armor); }
void UInventoryWidget::OnFilterConsumableClicked() { FilterByType(EItemType::Consumable); }
void UInventoryWidget::OnSortRarityClicked()       { SortItems(ESortMode::ByRarity); }
void UInventoryWidget::OnSortNameClicked()         { SortItems(ESortMode::ByName); }

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

    UItemSubsystem* FilterSub = (CurrentFilter != EItemType::All && GetWorld())
        ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;

    const bool bFiltering = (CurrentFilter != EItemType::All);

    TArray<FInventorySlot> Slots;
    TArray<int32> SourceIndices;   // 필터 시에만 채움(All이면 빈 배열 → 그리드 위치=인벤 인덱스)
    for (int32 i = 0; i < BoundInventory->GetMaxSlots(); ++i)
    {
        FInventorySlot S = BoundInventory->GetSlot(i);

        // 필터 적용: 종류가 안 맞는 아이템은 건너뜀(매칭 아이템이 앞으로 압축됨)
        if (bFiltering && S.IsEmpty() == false)
        {
            const FItemData* D = FilterSub ? FilterSub->GetItemData(S.ItemID) : nullptr;
            if (D == nullptr || D->ItemType != CurrentFilter)
            {
                continue;
            }
        }
        Slots.Add(S);
        if (bFiltering)
        {
            SourceIndices.Add(i);   // 표시 위치 → 실제 인벤 슬롯 인덱스 매핑
        }
    }

    GridWidget->RefreshGrid(Slots, SourceIndices);

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
    CurrentFilter = EItemType::All;   // 정렬은 항상 전체 아이템 대상(필터 리셋)
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
    Tooltip->SetVisibility(ESlateVisibility::HitTestInvisible);

    // 크기 계산 전 레이아웃 갱신(내용에 따라 desired size가 바뀌므로)
    Tooltip->ForceLayoutPrepass();

    // Tooltip은 InventoryWidget(→ScreenWidget) 안에 중첩돼 있으므로
    // 커서(절대 좌표)를 Tooltip 부모 캔버스의 "로컬 좌표"로 변환해야 위치가 맞음.
    UPanelWidget* TipParent = Tooltip->GetParent();
    const FGeometry ParentGeo = TipParent ? TipParent->GetCachedGeometry() : GetCachedGeometry();
    const FVector2D AbsCursor = FSlateApplication::Get().GetCursorPos();
    const FVector2D Local     = ParentGeo.AbsoluteToLocal(AbsCursor);
    const FVector2D AreaSize  = ParentGeo.GetLocalSize();
    const FVector2D TipSize   = Tooltip->GetDesiredSize();
    const FVector2D Margin(16.f, 16.f);

    // 기본은 커서 우하단. 영역 밖으로 나가면 반대쪽으로 뒤집고, 그래도 넘치면 안쪽으로 클램프
    FVector2D Pos = Local + Margin;
    if (Pos.X + TipSize.X > AreaSize.X) { Pos.X = Local.X - Margin.X - TipSize.X; }
    if (Pos.Y + TipSize.Y > AreaSize.Y) { Pos.Y = Local.Y - Margin.Y - TipSize.Y; }
    Pos.X = FMath::Clamp(Pos.X, 0.f, FMath::Max(0.f, AreaSize.X - TipSize.X));
    Pos.Y = FMath::Clamp(Pos.Y, 0.f, FMath::Max(0.f, AreaSize.Y - TipSize.Y));

    Tooltip->SetPosition(Pos);
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
