#include "InventoryScreenWidget.h"
#include "UI/Inventory/InventoryWidget.h"
#include "UI/Inventory/EquipmentWidget.h"
#include "UI/Common/CompareTooltipWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "Components/TextBlock.h"

void UInventoryScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CachedInvWidget     = Cast<UInventoryWidget>(GetWidgetFromName(TEXT("InvWidget")));
    CachedEquipWidget   = Cast<UEquipmentWidget>(GetWidgetFromName(TEXT("EquipWidget")));
    CachedCompareWidget = Cast<UCompareTooltipWidget>(GetWidgetFromName(TEXT("CompareWidget")));

    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (!Char) return;

    UInventoryComponent*  InvComp   = Char->FindComponentByClass<UInventoryComponent>();
    UEquipmentComponent*  EquipComp = Char->FindComponentByClass<UEquipmentComponent>();

    if (CachedInvWidget   && InvComp)   CachedInvWidget->BindToInventory(InvComp);
    if (CachedEquipWidget && EquipComp) CachedEquipWidget->BindToEquipment(EquipComp);

    RefreshBottomBar();
}

void UInventoryScreenWidget::RefreshBottomBar()
{
    if (WeightText) WeightText->SetText(FText::FromString(TEXT("0 / 100")));
    if (GoldText)   GoldText->SetText(FText::FromString(TEXT("0 G")));
}

void UInventoryScreenWidget::OnCloseBtnClicked()
{
    DeactivateWidget();
}

void UInventoryScreenWidget::ShowCompareTooltip(int32 InvSlotIndex)
{
    if (!CachedCompareWidget) return;

    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (!Char) return;

    UInventoryComponent*  InvComp   = Char->FindComponentByClass<UInventoryComponent>();
    UEquipmentComponent*  EquipComp = Char->FindComponentByClass<UEquipmentComponent>();
    UItemSubsystem*       ItemSub   = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;
    if (!InvComp || !EquipComp || !ItemSub) return;

    const FInventorySlot& InvSlot = InvComp->GetSlot(InvSlotIndex);
    if (InvSlot.IsEmpty()) return;

    const FItemData* NewData = ItemSub->GetItemData(InvSlot.ItemID);
    if (!NewData || NewData->EquipSlot == EEquipSlot::None) return;

    int32 EquippedID = EquipComp->GetEquippedItemID(NewData->EquipSlot);
    const FItemData* EquippedData = EquippedID != 0 ? ItemSub->GetItemData(EquippedID) : nullptr;

    static FItemData EmptyItem;
    CachedCompareWidget->SetCompareData(*NewData, EquippedData ? *EquippedData : EmptyItem);
    CachedCompareWidget->SetVisibility(ESlateVisibility::Visible);
}
