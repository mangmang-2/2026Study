#include "TradeScreenWidget.h"
#include "UI/Trade/TradeWidget.h"
#include "UI/Inventory/InventoryWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/TradeComponent.h"
#include "GameFramework/Character.h"

void UTradeScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (!Char) return;

    UInventoryComponent* InvComp   = Char->FindComponentByClass<UInventoryComponent>();
    UTradeComponent*     TradeComp = Char->FindComponentByClass<UTradeComponent>();

    if (InvPanel   && InvComp)   InvPanel->BindToInventory(InvComp);
    if (TradePanel && TradeComp) TradePanel->BindToTrade(TradeComp);
}

void UTradeScreenWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void UTradeScreenWidget::OpenTrade(ACharacter* TargetPlayer)
{
    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (!Char) return;

    UTradeComponent* TradeComp = Char->FindComponentByClass<UTradeComponent>();
    if (TradeComp) TradeComp->RequestTrade(TargetPlayer);
}

void UTradeScreenWidget::BindToTradeComponent(UTradeComponent* InTradeComp)
{
    if (TradePanel) TradePanel->BindToTrade(InTradeComp);
}

void UTradeScreenWidget::OnCloseBtnClicked()
{
    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (Char)
    {
        UTradeComponent* TradeComp = Char->FindComponentByClass<UTradeComponent>();
        if (TradeComp && TradeComp->IsTrading()) TradeComp->CancelTrade();
    }
    DeactivateWidget();
}
