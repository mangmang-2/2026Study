#pragma once

#include "CoreMinimal.h"
#include "Item/ItemBase.h"
#include "ArmorItem.generated.h"

UCLASS()
class STUDYPROJECT_API AArmorItem : public AItemBase
{
    GENERATED_BODY()

public:
    AArmorItem();

    UFUNCTION(BlueprintPure, Category = "Item")
    int32 GetBaseDEF() const { return BaseDEF; }

    UFUNCTION(BlueprintPure, Category = "Item")
    EEquipSlot GetArmorSlot() const { return ArmorSlot; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
    int32 BaseDEF = 5;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
    EEquipSlot ArmorSlot = EEquipSlot::Body;
};
