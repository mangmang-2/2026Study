#include "ItemSlotWidget.h"
#include "UI/Common/ItemIconWidget.h"
#include "UI/Common/ItemDragDropOperation.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Blueprint/DragDropOperation.h"

void UItemSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    CachedIconWidget = Cast<UItemIconWidget>(GetWidgetFromName(TEXT("IconWidget")));
}

void UItemSlotWidget::SetItemData(const FItemData& Data, int32 InQuantity, int32 InEnhanceLevel)
{
    // NativeConstruct가 아직 안 돌았을 수 있으므로(동적 생성 슬롯) lazy 바인딩
    if (CachedIconWidget == nullptr)
    {
        CachedIconWidget = Cast<UItemIconWidget>(GetWidgetFromName(TEXT("IconWidget")));
    }

    CachedItemData    = Data;
    CachedQuantity    = InQuantity;
    CachedEnhanceLevel = InEnhanceLevel;
    bHasItem          = (Data.ItemID != 0 && InQuantity > 0);

    if (!bHasItem)
    {
        ClearSlot();
        return;
    }

    if (CachedIconWidget)
    {
        CachedIconWidget->SetItemData(Data);
    }

    if (QuantityText)
    {
        QuantityText->SetText(InQuantity > 1 ? FText::AsNumber(InQuantity) : FText::GetEmpty());
        QuantityText->SetVisibility(InQuantity > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (EnhanceText)
    {
        EnhanceText->SetText(InEnhanceLevel > 0
            ? FText::Format(FText::FromString(TEXT("+{0}")), InEnhanceLevel)
            : FText::GetEmpty());
        EnhanceText->SetVisibility(InEnhanceLevel > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UItemSlotWidget::ClearSlot()
{
    bHasItem           = false;
    CachedItemData     = FItemData{};
    CachedQuantity     = 0;
    CachedEnhanceLevel = 0;

    if (CachedIconWidget)   CachedIconWidget->Clear();
    if (QuantityText) QuantityText->SetVisibility(ESlateVisibility::Collapsed);
    if (EnhanceText)  EnhanceText->SetVisibility(ESlateVisibility::Collapsed);
    if (CooldownBar)  CooldownBar->SetVisibility(ESlateVisibility::Collapsed);
}

void UItemSlotWidget::SetCooldownPercent(float Percent)
{
    if (CooldownBar)
    {
        CooldownBar->SetPercent(Percent);
        CooldownBar->SetVisibility(Percent > 0.f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

FReply UItemSlotWidget::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::RightMouseButton && bHasItem)
    {
        OnSlotRightClicked.Broadcast(SlotIndex);
        return FReply::Handled();
    }
    if (Event.GetEffectingButton() == EKeys::LeftMouseButton && bHasItem)
    {
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }
    return Super::NativeOnPreviewMouseButtonDown(Geometry, Event);
}

void UItemSlotWidget::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
    Super::NativeOnMouseEnter(Geometry, Event);
    if (bHasItem) OnSlotHovered.Broadcast(SlotIndex);
}

void UItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& Event)
{
    Super::NativeOnMouseLeave(Event);
    OnSlotHovered.Broadcast(-1);
}

void UItemSlotWidget::NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& OutOperation)
{
    if (bHasItem == false)
    {
        return;
    }

    UItemDragDropOperation* Op = NewObject<UItemDragDropOperation>(this);
    Op->SourceContext   = SlotContext;
    Op->SourceSlotIndex = SlotIndex;

    // 드래그 비주얼: 아이템 아이콘이 커서를 따라다님
    UTexture2D* Tex = CachedItemData.Icon.IsNull() ? nullptr : CachedItemData.Icon.LoadSynchronous();
    if (Tex != nullptr)
    {
        UImage* DragVis = NewObject<UImage>(this);
        DragVis->SetBrushFromTexture(Tex);
        DragVis->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.85f));
        DragVis->SetDesiredSizeOverride(FVector2D(56.f, 56.f));
        Op->DefaultDragVisual = DragVis;
        Op->Pivot = EDragPivot::CenterCenter;
    }

    OutOperation = Op;
}

bool UItemSlotWidget::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
    UItemDragDropOperation* Op = Cast<UItemDragDropOperation>(Operation);
    if (!Op) return false;

    OnSlotDrop.Broadcast(Op->SourceContext, Op->SourceSlotIndex, SlotIndex);
    return true;
}
