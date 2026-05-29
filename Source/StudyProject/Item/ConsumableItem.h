#pragma once

#include "CoreMinimal.h"
#include "Item/ItemBase.h"
#include "ConsumableItem.generated.h"

UCLASS()
class STUDYPROJECT_API AConsumableItem : public AItemBase
{
    GENERATED_BODY()

public:
    AConsumableItem();

    UFUNCTION(BlueprintPure, Category = "Item")
    int32 GetHealAmount() const { return HealAmount; }

    UFUNCTION(BlueprintPure, Category = "Item")
    float GetCooldownTime() const { return CooldownTime; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
    int32 HealAmount = 50;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
    float CooldownTime = 3.f;
};
