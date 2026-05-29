#include "ShopWidget.h"
#include "UI/Common/ItemSlotWidget.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"

void UShopWidget::InitShop(int32 ShopID)
{
    CurrentShopID = ShopID;
    RefreshShopList();
}

void UShopWidget::RefreshShopList()
{
    if (!ShopList || !ShopSlotClass) return;
    ShopList->ClearChildren();

    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;
    if (!ItemSub) return;

    TArray<int32> Items = ItemSub->GetShopItems(CurrentShopID);
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        UItemSlotWidget* SlotW = CreateWidget<UItemSlotWidget>(this, ShopSlotClass);
        if (!SlotW) continue;

        SlotW->SlotIndex   = i;
        SlotW->SlotContext = ESlotContext::Shop;

        const FItemData* Data = ItemSub->GetItemData(Items[i]);
        if (Data) SlotW->SetItemData(*Data, 1, 0);

        SlotW->OnSlotRightClicked.AddDynamic(this, &UShopWidget::HandleShopItemClicked);
        ShopList->AddChild(SlotW);
    }
}

void UShopWidget::HandleShopItemClicked(int32 SlotIndex)
{
    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;
    if (!ItemSub) return;

    TArray<int32> Items = ItemSub->GetShopItems(CurrentShopID);
    if (Items.IsValidIndex(SlotIndex))
        OnBuyClicked.Broadcast(Items[SlotIndex], 1);
}
