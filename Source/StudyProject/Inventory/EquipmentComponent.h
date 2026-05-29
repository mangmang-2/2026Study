#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Data/ItemData.h"
#include "EquipmentComponent.generated.h"

// TMap은 리플리케이션 불가 → 슬롯별 int32 필드로 관리
USTRUCT(BlueprintType)
struct FEquippedItemsData
{
    GENERATED_BODY()

    UPROPERTY() 
    int32 HeadItemID   = 0;
    UPROPERTY() 
    int32 BodyItemID   = 0;
    UPROPERTY() 
    int32 WeaponItemID = 0;
    UPROPERTY() 
    int32 ShieldItemID = 0;

    int32  Get(EEquipSlot Slot) const;
    void   Set(EEquipSlot Slot, int32 ItemID);
    void   Clear(EEquipSlot Slot);
    bool   IsOccupied(EEquipSlot Slot) const { return Get(Slot) != 0; }

    TArray<TPair<EEquipSlot, int32>> GetAll() const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STUDYPROJECT_API UEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEquipmentComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool Equip(int32 ItemID, int32 FromInventorySlot);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool Unequip(EEquipSlot Slot);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    int32 GetEquippedItemID(EEquipSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool IsSlotOccupied(EEquipSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    int32 GetTotalBonusATK() const;

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    int32 GetTotalBonusDEF() const;

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    int32 GetTotalBonusHP() const;

    UPROPERTY(BlueprintAssignable, Category = "Equipment")
    FOnEquipmentChanged OnEquipmentChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_EquippedItems)
    FEquippedItemsData EquippedItems;

    UFUNCTION()
    void OnRep_EquippedItems();

private:
    UFUNCTION(Server, Reliable)
    void Server_Equip(int32 ItemID, int32 FromInventorySlot);

    UFUNCTION(Server, Reliable)
    void Server_Unequip(EEquipSlot Slot);

    void ApplyMeshForSlot(EEquipSlot Slot, int32 ItemID);
    void ClearMeshForSlot(EEquipSlot Slot);
};