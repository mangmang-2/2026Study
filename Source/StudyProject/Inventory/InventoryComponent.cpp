#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UInventoryComponent::UInventoryComponent()
{
    SetIsReplicatedByDefault(true);

    // 빈 슬롯 MaxSlots개로 초기화
    InventoryList.Slots.SetNum(MaxSlots);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UInventoryComponent, InventoryList);
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    // 생성자 SetNum은 C++ 기본값 기준 → BP에서 바꾼 MaxSlots에 맞춰 재조정
    if (GetOwner() != nullptr && GetOwner()->HasAuthority())
    {
        if (InventoryList.Slots.Num() != MaxSlots)
        {
            InventoryList.Slots.SetNum(MaxSlots);
            InventoryList.MarkArrayDirty();
        }
    }
}

// ── 공개 API ─────────────────────────────────────────────────────────

bool UInventoryComponent::AddItem(int32 ItemID, int32 Quantity)
{
    if (GetOwner()->HasAuthority())
    {
        return Internal_AddItem(ItemID, Quantity);
    }
    Server_AddItem(ItemID, Quantity);
    return true;
}

bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
    if (GetOwner()->HasAuthority())
    {
        if (!InventoryList.Slots.IsValidIndex(SlotIndex)) return false;

        FInventorySlot& Slot = InventoryList.Slots[SlotIndex];
        if (Slot.IsEmpty() || Slot.Quantity < Quantity) return false;

        Slot.Quantity -= Quantity;
        if (Slot.Quantity <= 0)
        {
            Slot.ItemID   = 0;
            Slot.Quantity = 0;

            // 빈칸이 생기면 뒤 아이템들을 앞으로 당김(compaction)
            TArray<FInventorySlot> Packed;
            for (const FInventorySlot& S : InventoryList.Slots)
            {
                if (S.IsEmpty() == false)
                {
                    Packed.Add(S);
                }
            }
            for (int32 i = 0; i < InventoryList.Slots.Num(); ++i)
            {
                if (i < Packed.Num())
                {
                    InventoryList.Slots[i] = Packed[i];
                }
                else
                {
                    InventoryList.Slots[i].ItemID       = 0;
                    InventoryList.Slots[i].Quantity     = 0;
                    InventoryList.Slots[i].EnhanceLevel = 0;
                }
                InventoryList.MarkItemDirty(InventoryList.Slots[i]);
            }
            OnInventoryChanged.Broadcast();
            return true;
        }
        InventoryList.MarkItemDirty(Slot);
        OnInventoryChanged.Broadcast();
        return true;
    }
    Server_RemoveItem(SlotIndex, Quantity);
    return true;
}

bool UInventoryComponent::MoveSlot(int32 FromIndex, int32 ToIndex)
{
    if (GetOwner()->HasAuthority())
    {
        if (!InventoryList.Slots.IsValidIndex(FromIndex) || !InventoryList.Slots.IsValidIndex(ToIndex)) return false;

        InventoryList.Slots.Swap(FromIndex, ToIndex);
        InventoryList.MarkItemDirty(InventoryList.Slots[FromIndex]);
        InventoryList.MarkItemDirty(InventoryList.Slots[ToIndex]);
        OnInventoryChanged.Broadcast();
        return true;
    }
    Server_MoveSlot(FromIndex, ToIndex);
    return true;
}

bool UInventoryComponent::SplitStack(int32 SlotIndex, int32 SplitQuantity)
{
    if (GetOwner()->HasAuthority())
    {
        if (!InventoryList.Slots.IsValidIndex(SlotIndex)) return false;
        FInventorySlot& Src = InventoryList.Slots[SlotIndex];
        if (Src.IsEmpty() || SplitQuantity <= 0 || SplitQuantity >= Src.Quantity) return false;

        int32 EmptySlot = FindEmptySlot();
        if (EmptySlot == -1) return false;

        FInventorySlot& Dst = InventoryList.Slots[EmptySlot];
        Dst.ItemID   = Src.ItemID;
        Dst.Quantity = SplitQuantity;
        Src.Quantity -= SplitQuantity;

        InventoryList.MarkItemDirty(Src);
        InventoryList.MarkItemDirty(Dst);
        OnInventoryChanged.Broadcast();
        return true;
    }
    Server_SplitStack(SlotIndex, SplitQuantity);
    return true;
}

bool UInventoryComponent::MergeStacks(int32 FromIndex, int32 ToIndex)
{
    if (GetOwner()->HasAuthority())
    {
        if (!InventoryList.Slots.IsValidIndex(FromIndex) || !InventoryList.Slots.IsValidIndex(ToIndex)) return false;

        FInventorySlot& From = InventoryList.Slots[FromIndex];
        FInventorySlot& To   = InventoryList.Slots[ToIndex];
        if (From.IsEmpty() || To.IsEmpty() || From.ItemID != To.ItemID) return false;

        UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
        const FItemData* Data = ItemSub ? ItemSub->GetItemData(From.ItemID) : nullptr;
        int32 MaxStack = Data ? Data->MaxStack : 1;

        int32 Transferable = FMath::Min(From.Quantity, MaxStack - To.Quantity);
        if (Transferable <= 0) return false;

        To.Quantity   += Transferable;
        From.Quantity -= Transferable;
        if (From.Quantity <= 0) { From.ItemID = 0; From.Quantity = 0; }

        InventoryList.MarkItemDirty(From);
        InventoryList.MarkItemDirty(To);
        OnInventoryChanged.Broadcast();
        return true;
    }
    Server_MergeStacks(FromIndex, ToIndex);
    return true;
}

bool UInventoryComponent::UseItem(int32 SlotIndex)
{
    if (GetOwner()->HasAuthority())
    {
        if (!InventoryList.Slots.IsValidIndex(SlotIndex)) return false;
        FInventorySlot& Slot = InventoryList.Slots[SlotIndex];
        if (Slot.IsEmpty()) return false;

        UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
        const FItemData* Data = ItemSub ? ItemSub->GetItemData(Slot.ItemID) : nullptr;
        if (!Data || Data->ItemType != EItemType::Consumable) return false;

        // 쿨타임 체크
        float* LastUse = ItemCooldowns.Find(Slot.ItemID);
        float Now = GetWorld()->GetTimeSeconds();
        if (LastUse && (Now - *LastUse) < Data->CooldownTime) return false;

        // 사용 처리 (HP 회복 등은 Character에서 처리)
        ItemCooldowns.Add(Slot.ItemID, Now);
        RemoveItem(SlotIndex, 1);
        return true;
    }
    Server_UseItem(SlotIndex);
    return true;
}

// ── 탐색 ─────────────────────────────────────────────────────────────

int32 UInventoryComponent::FindItemByID(int32 ItemID) const
{
    for (int32 i = 0; i < InventoryList.Slots.Num(); ++i)
    {
        if (InventoryList.Slots[i].ItemID == ItemID) return i;
    }
    return -1;
}

int32 UInventoryComponent::FindEmptySlot() const
{
    for (int32 i = 0; i < InventoryList.Slots.Num(); ++i)
    {
        if (InventoryList.Slots[i].IsEmpty()) return i;
    }
    return -1;
}

bool UInventoryComponent::HasItem(int32 ItemID, int32 Quantity) const
{
    int32 Total = 0;
    for (const FInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemID == ItemID) Total += Slot.Quantity;
    }
    return Total >= Quantity;
}

bool UInventoryComponent::IsFull() const
{
    return FindEmptySlot() == -1;
}

const FInventorySlot& UInventoryComponent::GetSlot(int32 Index) const
{
    static FInventorySlot EmptySlot;
    if (!InventoryList.Slots.IsValidIndex(Index)) return EmptySlot;
    return InventoryList.Slots[Index];
}

// ── 필터 / 정렬 ───────────────────────────────────────────────────────

TArray<int32> UInventoryComponent::GetFilteredSlots(EItemType Type) const
{
    TArray<int32> Result;
    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return Result;

    for (int32 i = 0; i < InventoryList.Slots.Num(); ++i)
    {
        const FInventorySlot& Slot = InventoryList.Slots[i];
        if (Slot.IsEmpty()) continue;

        const FItemData* Data = ItemSub->GetItemData(Slot.ItemID);
        if (Data && Data->ItemType == Type) Result.Add(i);
    }
    return Result;
}

void UInventoryComponent::SortByRarity()
{
    if (!GetOwner()->HasAuthority()) return;

    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return;

    InventoryList.Slots.Sort([&](const FInventorySlot& A, const FInventorySlot& B)
    {
        if (A.IsEmpty()) return false;
        if (B.IsEmpty()) return true;
        const FItemData* DataA = ItemSub->GetItemData(A.ItemID);
        const FItemData* DataB = ItemSub->GetItemData(B.ItemID);
        if (!DataA) return false;
        if (!DataB) return true;
        return (uint8)DataA->Rarity > (uint8)DataB->Rarity;
    });

    InventoryList.MarkArrayDirty();
    OnInventoryChanged.Broadcast();
}

void UInventoryComponent::SortByName()
{
    if (!GetOwner()->HasAuthority()) return;

    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return;

    InventoryList.Slots.Sort([&](const FInventorySlot& A, const FInventorySlot& B)
    {
        if (A.IsEmpty()) return false;
        if (B.IsEmpty()) return true;
        const FItemData* DataA = ItemSub->GetItemData(A.ItemID);
        const FItemData* DataB = ItemSub->GetItemData(B.ItemID);
        if (!DataA) return false;
        if (!DataB) return true;
        return DataA->ItemName.ToString() < DataB->ItemName.ToString();
    });

    InventoryList.MarkArrayDirty();
    OnInventoryChanged.Broadcast();
}

void UInventoryComponent::SetSlotEnhanceLevel(int32 SlotIndex, int32 NewLevel)
{
    if (!GetOwner()->HasAuthority()) return;
    if (!InventoryList.Slots.IsValidIndex(SlotIndex)) return;

    FInventorySlot& Slot = InventoryList.Slots[SlotIndex];
    if (Slot.IsEmpty()) return;

    Slot.EnhanceLevel = NewLevel;
    InventoryList.MarkItemDirty(Slot);
    OnInventoryChanged.Broadcast();
}

bool UInventoryComponent::AddItemWithEnhance(int32 ItemID, int32 EnhanceLevel)
{
    if (!GetOwner()->HasAuthority())
    {
        return false;
    }
    int32 EmptySlot = FindEmptySlot();
    if (EmptySlot == -1)
    {
        return false;
    }
    FInventorySlot& Slot = InventoryList.Slots[EmptySlot];
    Slot.ItemID = ItemID;
    Slot.Quantity = 1;
    Slot.EnhanceLevel = EnhanceLevel;
    InventoryList.MarkItemDirty(Slot);
    OnInventoryChanged.Broadcast();
    return true;
}

// ── Server RPC 구현 ───────────────────────────────────────────────────

void UInventoryComponent::Server_AddItem_Implementation(int32 ItemID, int32 Quantity)       { Internal_AddItem(ItemID, Quantity); }
void UInventoryComponent::Server_RemoveItem_Implementation(int32 SlotIndex, int32 Quantity) { RemoveItem(SlotIndex, Quantity); }
void UInventoryComponent::Server_MoveSlot_Implementation(int32 FromIndex, int32 ToIndex)    { MoveSlot(FromIndex, ToIndex); }
void UInventoryComponent::Server_SplitStack_Implementation(int32 SlotIndex, int32 Qty)      { SplitStack(SlotIndex, Qty); }
void UInventoryComponent::Server_MergeStacks_Implementation(int32 From, int32 To)           { MergeStacks(From, To); }
void UInventoryComponent::Server_UseItem_Implementation(int32 SlotIndex)                    { UseItem(SlotIndex); }

// ── Internal ─────────────────────────────────────────────────────────

bool UInventoryComponent::Internal_AddItem(int32 ItemID, int32 Quantity)
{
    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    const FItemData* Data = ItemSub ? ItemSub->GetItemData(ItemID) : nullptr;
    int32 MaxStack = Data ? Data->MaxStack : 1;

    int32 Remaining = Quantity;

    // 기존 스택에 합치기 시도
    if (MaxStack > 1)
    {
        for (FInventorySlot& Slot : InventoryList.Slots)
        {
            if (Slot.ItemID != ItemID) continue;
            int32 CanAdd = MaxStack - Slot.Quantity;
            if (CanAdd <= 0) continue;

            int32 Adding = FMath::Min(Remaining, CanAdd);
            Slot.Quantity += Adding;
            Remaining -= Adding;
            InventoryList.MarkItemDirty(Slot);
            if (Remaining <= 0) break;
        }
    }

    // 남은 수량 새 슬롯에 추가
    while (Remaining > 0)
    {
        int32 EmptySlot = FindEmptySlot();
        if (EmptySlot == -1) return false; // 인벤 가득 참

        FInventorySlot& Slot = InventoryList.Slots[EmptySlot];
        int32 Adding = FMath::Min(Remaining, MaxStack);
        Slot.ItemID   = ItemID;
        Slot.Quantity = Adding;
        Remaining -= Adding;
        InventoryList.MarkItemDirty(Slot);
    }

    OnInventoryChanged.Broadcast();
    return true;
}
