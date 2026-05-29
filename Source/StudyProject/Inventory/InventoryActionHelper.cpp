#include "InventoryActionHelper.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/TradeComponent.h"
#include "GameFramework/Character.h"

void UInventoryActionHelper::HandleDrop(
    ESlotContext From, ESlotContext To,
    int32 FromSlot, int32 ToSlot,
    ACharacter* PlayerChar)
{
    if (!PlayerChar) return;

    UInventoryComponent* InvComp  = PlayerChar->FindComponentByClass<UInventoryComponent>();
    UEquipmentComponent* EquipComp = PlayerChar->FindComponentByClass<UEquipmentComponent>();
    UTradeComponent*     TradeComp = PlayerChar->FindComponentByClass<UTradeComponent>();

    // 인벤 → 인벤: 슬롯 이동
    if (From == ESlotContext::Inventory && To == ESlotContext::Inventory)
    {
        if (InvComp) InvComp->MoveSlot(FromSlot, ToSlot);
        return;
    }

    // 인벤 → 장비: 장착
    if (From == ESlotContext::Inventory && To == ESlotContext::Equipment)
    {
        const FInventorySlot& Slot = InvComp ? InvComp->GetSlot(FromSlot) : FInventorySlot{};
        if (EquipComp && !Slot.IsEmpty())
            EquipComp->Equip(Slot.ItemID, FromSlot);
        return;
    }

    // 장비 → 인벤: 장착 해제
    if (From == ESlotContext::Equipment && To == ESlotContext::Inventory)
    {
        if (EquipComp)
            EquipComp->Unequip(static_cast<EEquipSlot>(FromSlot));
        return;
    }

    // 인벤 → 강화 대상
    if (From == ESlotContext::Inventory && To == ESlotContext::EnhanceTarget)
    {
        // UI 레이어에서 처리 (EnhanceWidget이 슬롯 인덱스 보관)
        return;
    }

    // 인벤 → 거래 등록
    if (From == ESlotContext::Inventory && To == ESlotContext::TradeRegister)
    {
        const FInventorySlot& Slot = InvComp ? InvComp->GetSlot(FromSlot) : FInventorySlot{};
        if (TradeComp && !Slot.IsEmpty())
            TradeComp->RegisterItem(FromSlot, Slot.Quantity);
        return;
    }

    // 거래 → 인벤: 등록 취소
    if (From == ESlotContext::TradeRegister && To == ESlotContext::Inventory)
    {
        if (TradeComp) TradeComp->UnregisterItem(FromSlot);
        return;
    }
}
