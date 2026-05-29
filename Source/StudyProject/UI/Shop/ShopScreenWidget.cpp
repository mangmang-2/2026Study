#include "ShopScreenWidget.h"
#include "UI/Shop/ShopWidget.h"
#include "UI/Inventory/InventoryWidget.h"
#include "Inventory/InventoryComponent.h"
#include "GameFramework/Character.h"
#include "Components/TextBlock.h"

void UShopScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (Char == nullptr)
    {
        return;
    }

    UInventoryComponent* InvComp = Char->FindComponentByClass<UInventoryComponent>();
    if (InvPanel != nullptr && InvComp != nullptr)
    {
        InvPanel->BindToInventory(InvComp);
    }

    if (ShopPanel != nullptr)
    {
        ShopPanel->OnBuyClicked.AddDynamic(this, &UShopScreenWidget::HandleBuy);
        ShopPanel->OnSellClicked.AddDynamic(this, &UShopScreenWidget::HandleSell);
    }
}

void UShopScreenWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (ShopPanel != nullptr)
    {
        ShopPanel->OnBuyClicked.RemoveDynamic(this, &UShopScreenWidget::HandleBuy);
        ShopPanel->OnSellClicked.RemoveDynamic(this, &UShopScreenWidget::HandleSell);
    }
}

void UShopScreenWidget::SetShopID(int32 ShopID)
{
    if (ShopPanel != nullptr)
    {
        ShopPanel->InitShop(ShopID);
    }
}

void UShopScreenWidget::RefreshGold()
{
    if (GoldText != nullptr)
    {
        GoldText->SetText(FText::FromString(TEXT("0 G")));
    }
}

void UShopScreenWidget::OnCloseBtnClicked()
{
    DeactivateWidget();
}

void UShopScreenWidget::HandleBuy(int32 ItemID, int32 Quantity)
{
    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (Char == nullptr)
    {
        return;
    }

    UInventoryComponent* InvComp = Char->FindComponentByClass<UInventoryComponent>();
    if (InvComp != nullptr)
    {
        InvComp->AddItem(ItemID, Quantity);
    }

    RefreshGold();
}

void UShopScreenWidget::HandleSell(int32 InvSlot, int32 Quantity)
{
    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (Char == nullptr)
    {
        return;
    }

    UInventoryComponent* InvComp = Char->FindComponentByClass<UInventoryComponent>();
    if (InvComp != nullptr)
    {
        InvComp->RemoveItem(InvSlot, Quantity);
    }

    RefreshGold();
}
