#include "ComboData.h"
#include "Inventory/EquipmentComponent.h"
#include "Data/ItemData.h"

bool UComboLibrary::GetWeaponComboData(AActor* Avatar, UDataTable* Table, FName DefaultRow, FWeaponComboData& OutData)
{
    if (Table == nullptr)
    {
        return false;
    }

    FName RowName = DefaultRow;

    // 장착 무기 ItemID로 행을 먼저 시도
    if (Avatar != nullptr)
    {
        if (UEquipmentComponent* Equip = Avatar->FindComponentByClass<UEquipmentComponent>())
        {
            const int32 WeaponID = Equip->GetEquippedItemID(EEquipSlot::Weapon);
            if (WeaponID != 0)
            {
                const FName WeaponRow(*FString::FromInt(WeaponID));
                if (Table->FindRow<FWeaponComboData>(WeaponRow, TEXT("ComboLookup"), false) != nullptr)
                {
                    RowName = WeaponRow;
                }
            }
        }
    }

    const FWeaponComboData* Row = Table->FindRow<FWeaponComboData>(RowName, TEXT("ComboLookup"), false);
    if (Row == nullptr)
    {
        return false;
    }
    OutData = *Row;
    return true;
}
