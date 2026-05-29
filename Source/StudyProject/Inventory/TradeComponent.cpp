#include "TradeComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

UTradeComponent::UTradeComponent()
{
    SetIsReplicatedByDefault(true);
}

void UTradeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UTradeComponent, MyOffer);
    DOREPLIFETIME(UTradeComponent, PartnerOffer);
    DOREPLIFETIME(UTradeComponent, PartnerActor);
}

void UTradeComponent::RequestTrade(ACharacter* TargetPlayer)
{
    if (GetOwner()->HasAuthority()) { Server_RequestTrade_Implementation(TargetPlayer); return; }
    Server_RequestTrade(TargetPlayer);
}

void UTradeComponent::RegisterItem(int32 InvSlotIndex, int32 Quantity)
{
    if (!IsTrading()) return;
    if (GetOwner()->HasAuthority()) { Server_RegisterItem_Implementation(InvSlotIndex, Quantity); return; }
    Server_RegisterItem(InvSlotIndex, Quantity);
}

void UTradeComponent::UnregisterItem(int32 TradeSlotIndex)
{
    if (!IsTrading()) return;
    if (GetOwner()->HasAuthority()) { Server_UnregisterItem_Implementation(TradeSlotIndex); return; }
    Server_UnregisterItem(TradeSlotIndex);
}

void UTradeComponent::ConfirmTrade()
{
    if (!IsTrading()) return;
    if (GetOwner()->HasAuthority()) { Server_ConfirmTrade_Implementation(); return; }
    Server_ConfirmTrade();
}

void UTradeComponent::CancelTrade()
{
    if (GetOwner()->HasAuthority()) { Server_CancelTrade_Implementation(); return; }
    Server_CancelTrade();
}

void UTradeComponent::Server_RequestTrade_Implementation(ACharacter* Target)
{
    if (!Target) return;
    UTradeComponent* PartnerComp = Target->FindComponentByClass<UTradeComponent>();
    if (!PartnerComp || PartnerComp->IsTrading()) return;

    PartnerActor = Target;
    PartnerComp->PartnerActor = GetOwner();
    OnTradeUpdated.Broadcast();
    PartnerComp->OnTradeUpdated.Broadcast();
}

void UTradeComponent::Server_RegisterItem_Implementation(int32 InvSlotIndex, int32 Quantity)
{
    UInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UInventoryComponent>();
    if (!InvComp) return;

    const FInventorySlot& Slot = InvComp->GetSlot(InvSlotIndex);
    if (Slot.IsEmpty() || Slot.Quantity < Quantity) return;

    FTradeSlot TradeSlot;
    TradeSlot.ItemID    = Slot.ItemID;
    TradeSlot.Quantity  = Quantity;
    TradeSlot.SlotIndex = InvSlotIndex;
    MyOffer.Slots.Add(TradeSlot);
    MyOffer.bConfirmed = false;

    if (PartnerActor)
    {
        UTradeComponent* PartnerComp = PartnerActor->FindComponentByClass<UTradeComponent>();
        if (PartnerComp)
        {
            PartnerComp->PartnerOffer = MyOffer;
            PartnerComp->OnTradeUpdated.Broadcast();
        }
    }
    OnTradeUpdated.Broadcast();
}

void UTradeComponent::Server_UnregisterItem_Implementation(int32 TradeSlotIndex)
{
    if (!MyOffer.Slots.IsValidIndex(TradeSlotIndex)) return;
    MyOffer.Slots.RemoveAt(TradeSlotIndex);
    MyOffer.bConfirmed = false;

    if (PartnerActor)
    {
        UTradeComponent* PartnerComp = PartnerActor->FindComponentByClass<UTradeComponent>();
        if (PartnerComp)
        {
            PartnerComp->PartnerOffer = MyOffer;
            PartnerComp->OnTradeUpdated.Broadcast();
        }
    }
    OnTradeUpdated.Broadcast();
}

void UTradeComponent::Server_ConfirmTrade_Implementation()
{
    MyOffer.bConfirmed = true;

    UTradeComponent* PartnerComp = PartnerActor ? PartnerActor->FindComponentByClass<UTradeComponent>() : nullptr;
    if (PartnerComp && PartnerComp->MyOffer.bConfirmed)
    {
        Internal_ExecuteTrade();
    }
    else
    {
        if (PartnerComp)
        {
            PartnerComp->PartnerOffer = MyOffer;
            PartnerComp->OnTradeUpdated.Broadcast();
        }
        OnTradeUpdated.Broadcast();
    }
}

void UTradeComponent::Server_CancelTrade_Implementation()
{
    UTradeComponent* PartnerComp = PartnerActor ? PartnerActor->FindComponentByClass<UTradeComponent>() : nullptr;
    if (PartnerComp)
    {
        PartnerComp->PartnerActor = nullptr;
        PartnerComp->MyOffer      = FTradeOffer{};
        PartnerComp->PartnerOffer = FTradeOffer{};
        PartnerComp->OnTradeResult.Broadcast(false);
    }

    PartnerActor = nullptr;
    MyOffer      = FTradeOffer{};
    PartnerOffer = FTradeOffer{};
    OnTradeResult.Broadcast(false);
}

void UTradeComponent::Internal_ExecuteTrade()
{
    UTradeComponent* PartnerComp = PartnerActor ? PartnerActor->FindComponentByClass<UTradeComponent>() : nullptr;
    if (!PartnerComp) return;

    UInventoryComponent* MyInv      = GetOwner()->FindComponentByClass<UInventoryComponent>();
    UInventoryComponent* PartnerInv = PartnerActor->FindComponentByClass<UInventoryComponent>();
    if (!MyInv || !PartnerInv) return;

    // 내 아이템 제거 후 상대 인벤에 추가
    for (const FTradeSlot& TS : MyOffer.Slots)
    {
        MyInv->RemoveItem(TS.SlotIndex, TS.Quantity);
        PartnerInv->AddItem(TS.ItemID, TS.Quantity);
    }

    // 상대 아이템 제거 후 내 인벤에 추가
    for (const FTradeSlot& TS : PartnerComp->MyOffer.Slots)
    {
        PartnerInv->RemoveItem(TS.SlotIndex, TS.Quantity);
        MyInv->AddItem(TS.ItemID, TS.Quantity);
    }

    PartnerComp->PartnerActor = nullptr;
    PartnerComp->MyOffer      = FTradeOffer{};
    PartnerComp->PartnerOffer = FTradeOffer{};
    PartnerComp->OnTradeResult.Broadcast(true);

    PartnerActor = nullptr;
    MyOffer      = FTradeOffer{};
    PartnerOffer = FTradeOffer{};
    OnTradeResult.Broadcast(true);
}

void UTradeComponent::OnRep_MyOffer()      { OnTradeUpdated.Broadcast(); }
void UTradeComponent::OnRep_PartnerOffer() { OnTradeUpdated.Broadcast(); }
