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
    int32 HeadItemID     = 0;
    UPROPERTY()
    int32 BodyItemID     = 0;
    UPROPERTY()
    int32 HandsItemID    = 0;
    UPROPERTY()
    int32 LegsItemID     = 0;
    UPROPERTY()
    int32 FeetItemID     = 0;
    UPROPERTY()
    int32 ShoulderItemID = 0;
    UPROPERTY()
    int32 ArmsItemID     = 0;
    UPROPERTY()
    int32 WeaponItemID   = 0;
    UPROPERTY()
    int32 ShieldItemID   = 0;

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

    // 장착한 무기를 직접 강화(레벨 +Delta) → 즉시 오라 VFX 갱신(서버 권위, 모든 클라 복제).
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void EnhanceEquippedWeapon(int32 Delta = 1);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    int32 GetWeaponEnhanceLevel() const { return WeaponEnhanceLevel; }

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

    // 장착한 무기의 강화 레벨(>0이면 오라 VFX 표시). 장착 시 인벤 슬롯에서 캡처.
    UPROPERTY(ReplicatedUsing = OnRep_WeaponEnhanceLevel)
    int32 WeaponEnhanceLevel = 0;

    UFUNCTION()
    void OnRep_WeaponEnhanceLevel();

private:
    UFUNCTION(Server, Reliable)
    void Server_Equip(int32 ItemID, int32 FromInventorySlot);

    UFUNCTION(Server, Reliable)
    void Server_Unequip(EEquipSlot Slot);

    UFUNCTION(Server, Reliable)
    void Server_EnhanceEquippedWeapon(int32 Delta);

    void ApplyMeshForSlot(EEquipSlot Slot, int32 ItemID);
    void ClearMeshForSlot(EEquipSlot Slot);

    // 장착 무기 강화 레벨/아이템에 따라 오라 VFX 갱신
    void UpdateWeaponAura();
};