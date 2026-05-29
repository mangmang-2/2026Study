#include "EquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Inventory/InventoryComponent.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

// ── FEquippedItemsData ────────────────────────────────────────────────

int32 FEquippedItemsData::Get(EEquipSlot Slot) const
{
    switch (Slot)
    {
    case EEquipSlot::Head:   return HeadItemID;
    case EEquipSlot::Body:   return BodyItemID;
    case EEquipSlot::Weapon: return WeaponItemID;
    case EEquipSlot::Shield: return ShieldItemID;
    default: return 0;
    }
}

void FEquippedItemsData::Set(EEquipSlot Slot, int32 ItemID)
{
    switch (Slot)
    {
    case EEquipSlot::Head:   HeadItemID   = ItemID; break;
    case EEquipSlot::Body:   BodyItemID   = ItemID; break;
    case EEquipSlot::Weapon: WeaponItemID = ItemID; break;
    case EEquipSlot::Shield: ShieldItemID = ItemID; break;
    default: break;
    }
}

void FEquippedItemsData::Clear(EEquipSlot Slot)
{
    Set(Slot, 0);
}

TArray<TPair<EEquipSlot, int32>> FEquippedItemsData::GetAll() const
{
    TArray<TPair<EEquipSlot, int32>> Result;
    if (HeadItemID   != 0) Result.Add({ EEquipSlot::Head,   HeadItemID });
    if (BodyItemID   != 0) Result.Add({ EEquipSlot::Body,   BodyItemID });
    if (WeaponItemID != 0) Result.Add({ EEquipSlot::Weapon, WeaponItemID });
    if (ShieldItemID != 0) Result.Add({ EEquipSlot::Shield, ShieldItemID });
    return Result;
}

// ── UEquipmentComponent ───────────────────────────────────────────────

UEquipmentComponent::UEquipmentComponent()
{
    SetIsReplicatedByDefault(true);
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UEquipmentComponent, EquippedItems);
}

bool UEquipmentComponent::Equip(int32 ItemID, int32 FromInventorySlot)
{
    if (GetOwner()->HasAuthority())
    {
        UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
        const FItemData* Data = ItemSub ? ItemSub->GetItemData(ItemID) : nullptr;
        if (!Data || Data->EquipSlot == EEquipSlot::None) return false;

        // 기존 장착 아이템 → 인벤으로 반환
        int32 OldID = EquippedItems.Get(Data->EquipSlot);
        if (OldID != 0)
        {
            UInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UInventoryComponent>();
            if (InvComp) InvComp->AddItem(OldID, 1);
        }

        // 인벤에서 제거 후 장착
        UInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UInventoryComponent>();
        if (InvComp) InvComp->RemoveItem(FromInventorySlot, 1);

        EquippedItems.Set(Data->EquipSlot, ItemID);
        ApplyMeshForSlot(Data->EquipSlot, ItemID);
        OnEquipmentChanged.Broadcast();
        return true;
    }
    Server_Equip(ItemID, FromInventorySlot);
    return true;
}

bool UEquipmentComponent::Unequip(EEquipSlot Slot)
{
    if (GetOwner()->HasAuthority())
    {
        int32 EquippedID = EquippedItems.Get(Slot);
        if (EquippedID == 0) return false;

        UInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UInventoryComponent>();
        if (InvComp && !InvComp->IsFull())
        {
            InvComp->AddItem(EquippedID, 1);
        }

        ClearMeshForSlot(Slot);
        EquippedItems.Clear(Slot);
        OnEquipmentChanged.Broadcast();
        return true;
    }
    Server_Unequip(Slot);
    return true;
}

int32 UEquipmentComponent::GetEquippedItemID(EEquipSlot Slot) const
{
    return EquippedItems.Get(Slot);
}

bool UEquipmentComponent::IsSlotOccupied(EEquipSlot Slot) const
{
    return EquippedItems.IsOccupied(Slot);
}

int32 UEquipmentComponent::GetTotalBonusATK() const
{
    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return 0;

    int32 Total = 0;
    for (const auto& Pair : EquippedItems.GetAll())
    {
        const FItemData* Data = ItemSub->GetItemData(Pair.Value);
        if (Data) Total += Data->BaseATK;
    }
    return Total;
}

int32 UEquipmentComponent::GetTotalBonusDEF() const
{
    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return 0;

    int32 Total = 0;
    for (const auto& Pair : EquippedItems.GetAll())
    {
        const FItemData* Data = ItemSub->GetItemData(Pair.Value);
        if (Data) Total += Data->BaseDEF;
    }
    return Total;
}

int32 UEquipmentComponent::GetTotalBonusHP() const
{
    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return 0;

    int32 Total = 0;
    for (const auto& Pair : EquippedItems.GetAll())
    {
        const FItemData* Data = ItemSub->GetItemData(Pair.Value);
        if (Data) Total += Data->BaseHP;
    }
    return Total;
}

void UEquipmentComponent::OnRep_EquippedItems()
{
    for (const auto& Pair : EquippedItems.GetAll())
    {
        ApplyMeshForSlot(Pair.Key, Pair.Value);
    }
    OnEquipmentChanged.Broadcast();
}

void UEquipmentComponent::Server_Equip_Implementation(int32 ItemID, int32 FromInventorySlot)
{
    Equip(ItemID, FromInventorySlot);
}

void UEquipmentComponent::Server_Unequip_Implementation(EEquipSlot Slot)
{
    Unequip(Slot);
}

void UEquipmentComponent::ApplyMeshForSlot(EEquipSlot Slot, int32 ItemID)
{
    // TODO: Modular Character 메시 할당 (CharacterBase::SlotMeshMap 연동 후 구현)
}

void UEquipmentComponent::ClearMeshForSlot(EEquipSlot Slot)
{
    // TODO: 해당 슬롯 메시 제거
}