#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Data/ItemData.h"
#include "InventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FInventorySlot : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY() 
    int32 ItemID    = 0;
    UPROPERTY() 
    int32 Quantity  = 0;
    UPROPERTY() 
    int32 EnhanceLevel = 0;

    bool IsEmpty() const { return ItemID == 0 || Quantity <= 0; }
};

USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY() 
    TArray<FInventorySlot> Slots;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FInventorySlot, FInventoryList>(Slots, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
    enum { WithNetDeltaSerializer = true };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STUDYPROJECT_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ── 공개 API ────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(int32 ItemID, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool MoveSlot(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool SplitStack(int32 SlotIndex, int32 SplitQuantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool MergeStacks(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UseItem(int32 SlotIndex);

    // 슬롯 탐색
    int32 FindItemByID(int32 ItemID) const;
    int32 FindEmptySlot() const;
    bool  HasItem(int32 ItemID, int32 Quantity = 1) const;

    // 필터 / 정렬
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TArray<int32> GetFilteredSlots(EItemType Type) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SortByRarity();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SortByName();

    // 강화 등에서 특정 슬롯의 강화 레벨을 설정(서버 권위, C++ 전용)
    void SetSlotEnhanceLevel(int32 SlotIndex, int32 NewLevel);

    // 슬롯 조회
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    const FInventorySlot& GetSlot(int32 Index) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 GetMaxSlots() const { return MaxSlots; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool IsFull() const;

    // ── 이벤트 ──────────────────────────────────────────────────────
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryChanged OnInventoryChanged;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    int32 MaxSlots = 70;

    UPROPERTY(Replicated)
    FInventoryList InventoryList;

private:
    UFUNCTION(Server, Reliable)
    void Server_AddItem(int32 ItemID, int32 Quantity);

    UFUNCTION(Server, Reliable)
    void Server_RemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(Server, Reliable)
    void Server_MoveSlot(int32 FromIndex, int32 ToIndex);

    UFUNCTION(Server, Reliable)
    void Server_SplitStack(int32 SlotIndex, int32 SplitQuantity);

    UFUNCTION(Server, Reliable)
    void Server_MergeStacks(int32 FromIndex, int32 ToIndex);

    UFUNCTION(Server, Reliable)
    void Server_UseItem(int32 SlotIndex);

    bool Internal_AddItem(int32 ItemID, int32 Quantity);

    TMap<int32, float> ItemCooldowns;
};
