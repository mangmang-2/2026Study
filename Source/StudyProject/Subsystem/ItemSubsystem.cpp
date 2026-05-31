#include "ItemSubsystem.h"
#include "Engine/DataTable.h"
#include "Item/ItemBase.h"
#include "Engine/World.h"
#include "Data/GameData.h"

void UItemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    static const FSoftObjectPath ItemTablePath(TEXT("/Game/Data/DT_ItemData.DT_ItemData"));
    static const FSoftObjectPath DropTablePath(TEXT("/Game/Data/DT_DropTable.DT_DropTable"));
    static const FSoftObjectPath ShopTablePath(TEXT("/Game/Data/DT_ShopInventory.DT_ShopInventory"));

    ItemDataTable    = Cast<UDataTable>(ItemTablePath.TryLoad());
    DropTableData    = Cast<UDataTable>(DropTablePath.TryLoad());
    ShopInventoryData = Cast<UDataTable>(ShopTablePath.TryLoad());
}

const FItemData* UItemSubsystem::GetItemData(int32 ItemID) const
{
    if (!ItemDataTable) return nullptr;

    const FString RowName = FString::FromInt(ItemID);
    return ItemDataTable->FindRow<FItemData>(FName(*RowName), TEXT("GetItemData"));
}

TArray<FItemDrop> UItemSubsystem::RollDropTable(int32 MonsterID) const
{
    TArray<FItemDrop> Result;
    if (!DropTableData) return Result;

    TArray<FDropTableRow*> AllRows;
    DropTableData->GetAllRows<FDropTableRow>(TEXT("RollDropTable"), AllRows);

    for (const FDropTableRow* Row : AllRows)
    {
        if (!Row || Row->MonsterID != MonsterID) continue;

        if (FMath::FRand() <= Row->DropRate)
        {
            FItemDrop Drop;
            Drop.ItemID   = Row->ItemID;
            Drop.Quantity = FMath::RandRange(Row->MinQuantity, Row->MaxQuantity);
            Result.Add(Drop);
        }
    }
    return Result;
}

AItemBase* UItemSubsystem::SpawnItemInWorld(int32 ItemID, int32 Quantity, const FVector& Location, UWorld* World) const
{
    if (!World) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AItemBase* SpawnedItem = World->SpawnActor<AItemBase>(AItemBase::StaticClass(), Location, FRotator::ZeroRotator, Params);
    if (SpawnedItem)
    {
        SpawnedItem->InitItem(ItemID, Quantity);
    }
    return SpawnedItem;
}

TArray<int32> UItemSubsystem::GetShopItems(int32 ShopID) const
{
    TArray<int32> Result;
    if (!ShopInventoryData) return Result;

    // DT_ShopInventory 행을 ShopID로 필터링해 판매 아이템 ID 목록 반환
    for (const FName& RowName : ShopInventoryData->GetRowNames())
    {
        const FShopInventoryRow* Row = ShopInventoryData->FindRow<FShopInventoryRow>(RowName, TEXT("GetShopItems"));
        if (Row && Row->ShopID == ShopID && Row->ItemID != 0)
        {
            Result.Add(Row->ItemID);
        }
    }
    return Result;
}

TArray<int32> UItemSubsystem::GetAllItemIDs() const
{
    TArray<int32> Result;
    if (!ItemDataTable) return Result;

    for (const FName& RowName : ItemDataTable->GetRowNames())
    {
        const int32 ID = FCString::Atoi(*RowName.ToString());
        if (ID != 0) Result.Add(ID);
    }
    return Result;
}
