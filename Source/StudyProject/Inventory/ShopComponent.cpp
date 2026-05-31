#include "ShopComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Character/CharacterBase.h"
#include "Subsystem/ItemSubsystem.h"
#include "Data/ItemData.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"

UShopComponent::UShopComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UShopComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UShopComponent, ShopItemIDs);
    DOREPLIFETIME(UShopComponent, BuybackItemIDs);
}

// ── 헬퍼 ──────────────────────────────────────────────────────────────
UItemSubsystem* UShopComponent::GetItemSub() const
{
    return GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;
}

UInventoryComponent* UShopComponent::GetInv() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
}

ACharacterBase* UShopComponent::GetOwnerChar() const
{
    return Cast<ACharacterBase>(GetOwner());
}

void UShopComponent::NotifyChanged()
{
    OnShopChanged.Broadcast();
}

void UShopComponent::OnRep_Shop()
{
    OnShopChanged.Broadcast();
}

// ── 요청(클라/서버 라우팅) ────────────────────────────────────────────
void UShopComponent::RequestOpenShop(int32 ShopID)
{
    if (GetOwner()->HasAuthority()) { OpenShop_Internal(ShopID); }
    else                            { Server_OpenShop(ShopID); }
}

void UShopComponent::RequestBuy(int32 ShopIndex)
{
    if (GetOwner()->HasAuthority()) { Buy_Internal(ShopIndex); }
    else                            { Server_Buy(ShopIndex); }
}

void UShopComponent::RequestSell(int32 InvSlotIndex)
{
    if (GetOwner()->HasAuthority()) { Sell_Internal(InvSlotIndex); }
    else                            { Server_Sell(InvSlotIndex); }
}

void UShopComponent::RequestBuyback(int32 BuybackIndex)
{
    if (GetOwner()->HasAuthority()) { Buyback_Internal(BuybackIndex); }
    else                            { Server_Buyback(BuybackIndex); }
}

void UShopComponent::Server_OpenShop_Implementation(int32 ShopID) { OpenShop_Internal(ShopID); }
void UShopComponent::Server_Buy_Implementation(int32 ShopIndex)   { Buy_Internal(ShopIndex); }
void UShopComponent::Server_Sell_Implementation(int32 InvSlot)    { Sell_Internal(InvSlot); }
void UShopComponent::Server_Buyback_Implementation(int32 Index)   { Buyback_Internal(Index); }

// ── 서버 권위 로직 ────────────────────────────────────────────────────
void UShopComponent::OpenShop_Internal(int32 ShopID)
{
    UItemSubsystem* ItemSub = GetItemSub();
    if (!ItemSub) return;

    ActiveShopID = ShopID;
    ShopItemIDs.Reset();

    // 1) 기본 판매 목록
    ShopItemIDs.Append(ItemSub->GetShopItems(ShopID));

    // 2) 전체 아이템에서 랜덤 추가(중복 없이)
    TArray<int32> Pool = ItemSub->GetAllItemIDs();
    for (int32 i = Pool.Num() - 1; i > 0; --i)
    {
        Pool.Swap(i, FMath::RandRange(0, i));
    }
    int32 Added = 0;
    for (int32 i = 0; i < Pool.Num() && Added < RandomItemCount; ++i)
    {
        ShopItemIDs.AddUnique(Pool[i]);
        ++Added;
    }

    NotifyChanged();
}

void UShopComponent::Buy_Internal(int32 ShopIndex)
{
    if (!ShopItemIDs.IsValidIndex(ShopIndex)) return;

    UItemSubsystem* ItemSub = GetItemSub();
    UInventoryComponent* Inv = GetInv();
    ACharacterBase* PC = GetOwnerChar();
    if (!ItemSub || !Inv || !PC) return;

    const int32 ItemID = ShopItemIDs[ShopIndex];
    const FItemData* Data = ItemSub->GetItemData(ItemID);
    if (!Data) return;

    if (Inv->IsFull()) return;
    if (PC->GetGold() < Data->BuyPrice) return;

    if (PC->SpendGold(Data->BuyPrice))
    {
        Inv->AddItem(ItemID, 1);
        NotifyChanged();
    }
}

void UShopComponent::Sell_Internal(int32 InvSlotIndex)
{
    UItemSubsystem* ItemSub = GetItemSub();
    UInventoryComponent* Inv = GetInv();
    ACharacterBase* PC = GetOwnerChar();
    if (!ItemSub || !Inv || !PC) return;

    const FInventorySlot& InvItemSlot = Inv->GetSlot(InvSlotIndex);
    if (InvItemSlot.IsEmpty()) return;

    const int32 ItemID = InvItemSlot.ItemID;
    const FItemData* Data = ItemSub->GetItemData(ItemID);
    const int32 Gain = Data ? Data->SellPrice : 0;

    if (Inv->RemoveItem(InvSlotIndex, 1))
    {
        PC->AddGold(Gain);

        // 되사기 목록 맨 앞에 추가, 최대치 초과 시 오래된 것 제거
        BuybackItemIDs.Insert(ItemID, 0);
        while (BuybackItemIDs.Num() > MaxBuyback)
        {
            BuybackItemIDs.Pop();
        }
        NotifyChanged();
    }
}

void UShopComponent::Buyback_Internal(int32 BuybackIndex)
{
    if (!BuybackItemIDs.IsValidIndex(BuybackIndex)) return;

    UItemSubsystem* ItemSub = GetItemSub();
    UInventoryComponent* Inv = GetInv();
    ACharacterBase* PC = GetOwnerChar();
    if (!ItemSub || !Inv || !PC) return;

    const int32 ItemID = BuybackItemIDs[BuybackIndex];
    const FItemData* Data = ItemSub->GetItemData(ItemID);
    if (!Data) return;

    // 되사기 = 판매가로 재구매
    const int32 Cost = Data->SellPrice;
    if (Inv->IsFull()) return;
    if (PC->GetGold() < Cost) return;

    if (PC->SpendGold(Cost))
    {
        Inv->AddItem(ItemID, 1);
        BuybackItemIDs.RemoveAt(BuybackIndex);
        NotifyChanged();
    }
}
