#pragma once

#include "CoreMinimal.h"
#include "Data/SlotContext.h"
#include "InventoryActionHelper.generated.h"

UCLASS()
class STUDYPROJECT_API UInventoryActionHelper : public UObject
{
    GENERATED_BODY()

public:
    // From×To 컨텍스트에 따라 적절한 드랍 액션 수행
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    static void HandleDrop(
        ESlotContext From, ESlotContext To,
        int32 FromSlot, int32 ToSlot,
        ACharacter* PlayerChar);
};
