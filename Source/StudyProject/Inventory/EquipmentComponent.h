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

    // 슬롯별 강화 레벨(index = (uint8)EEquipSlot). 장착 시 인벤에서 캡처, 해제 시 인벤으로 복원.
    UPROPERTY()
    TArray<int32> EnhanceLevels;

    int32  Get(EEquipSlot Slot) const;
    void   Set(EEquipSlot Slot, int32 ItemID);
    void   Clear(EEquipSlot Slot);
    bool   IsOccupied(EEquipSlot Slot) const { return Get(Slot) != 0; }

    int32  GetEnhance(EEquipSlot Slot) const;
    void   SetEnhance(EEquipSlot Slot, int32 Level);

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

    // 장착 무기 직접 강화(+Delta) → 오라 VFX 갱신(서버 권위)
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void EnhanceEquippedWeapon(int32 Delta = 1);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    int32 GetWeaponEnhanceLevel() const { return WeaponEnhanceLevel; }

    // 장착된 슬롯의 강화 레벨(장비창 +N 표시용)
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    int32 GetEquippedEnhanceLevel(EEquipSlot Slot) const { return EquippedItems.GetEnhance(Slot); }

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool IsSlotOccupied(EEquipSlot Slot) const;

    // ── 장비 세트(로드아웃) ──────────────────────────────────────────
    static constexpr int32 NumLoadouts = 3;

    // 현재 장비 전체를 세트 Index에 저장
    UFUNCTION(BlueprintCallable, Category = "Equipment|Loadout")
    void SaveLoadout(int32 Index);

    // 세트 Index로 장비 일괄 교체(빈 세트면 전부 해제 = 빈 장비)
    UFUNCTION(BlueprintCallable, Category = "Equipment|Loadout")
    void ApplyLoadout(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Equipment|Loadout")
    bool IsLoadoutEmpty(int32 Index) const;

    // 현재 선택 세트(하이라이트/저장 대상). 장착·해제 시 이 세트에 자동 반영·저장.
    void SetActiveLoadout(int32 Index);
    int32 GetActiveLoadout() const { return ActiveLoadout; }

    // 세이브/로드용 — 세트 스냅샷 읽기/쓰기(쓰기는 서버 권위)
    FEquippedItemsData GetLoadout(int32 Index) const;
    void LoadLoadout(int32 Index, const FEquippedItemsData& Data);

    // 로드 직후 활성 세트를 현재 장비로 반영(메시 갱신, 디스크 저장 안 함)
    void InitFromActiveLoadout();

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

    // 저장된 장비 세트(NumLoadouts개). 인덱스별 전체 슬롯 스냅샷.
    UPROPERTY(Replicated)
    TArray<FEquippedItemsData> Loadouts;

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

    UFUNCTION(Server, Reliable)
    void Server_SaveLoadout(int32 Index);

    UFUNCTION(Server, Reliable)
    void Server_ApplyLoadout(int32 Index);

    void ApplyMeshForSlot(EEquipSlot Slot, int32 ItemID);
    void ClearMeshForSlot(EEquipSlot Slot);

    // 장착/해제 시: 현재 장비를 활성 세트에 반영 + 디스크 저장
    void OnEquipmentMutated();
    void PersistToDisk();
    void RefreshAllMeshes();   // EquippedItems 기준 전체 슬롯 메시 정리(빈 슬롯 포함)

    // 캐릭터가 항상 착용하는 활성 세트(하이라이트·장착 대상). 영속 저장됨.
    UPROPERTY(Replicated)
    int32 ActiveLoadout = 0;

    // 장착 무기 강화 레벨/아이템에 따라 오라 VFX 갱신
    void UpdateWeaponAura();
};