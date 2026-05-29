#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/ItemData.h"
#include "ItemSubsystem.generated.h"

class AItemBase;

UCLASS()
class STUDYPROJECT_API UItemSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // 아이템 데이터 조회
    const FItemData* GetItemData(int32 ItemID) const;

    // 드랍 테이블 확률 판정 → 드랍 목록 반환
    TArray<FItemDrop> RollDropTable(int32 MonsterID) const;

    // 월드에 아이템 액터 스폰
    AItemBase* SpawnItemInWorld(int32 ItemID, int32 Quantity, const FVector& Location, UWorld* World) const;

    // 상점 상품 목록 반환
    TArray<int32> GetShopItems(int32 ShopID) const;

private:
    UPROPERTY()
    TObjectPtr<UDataTable> ItemDataTable = nullptr;

    UPROPERTY()
    TObjectPtr<UDataTable> DropTableData = nullptr;

    UPROPERTY()
    TObjectPtr<UDataTable> ShopInventoryData = nullptr;
};
