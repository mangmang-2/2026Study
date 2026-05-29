#pragma once

#include "CoreMinimal.h"
#include "Item/ItemBase.h"
#include "WeaponItem.generated.h"

UCLASS()
class STUDYPROJECT_API AWeaponItem : public AItemBase
{
    GENERATED_BODY()

public:
    AWeaponItem();

    UFUNCTION(BlueprintPure, Category = "Item")
    int32 GetBaseATK() const { return BaseATK; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    int32 BaseATK = 10;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    EEquipSlot WeaponSlot = EEquipSlot::Weapon;
};
