#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Data/SlotContext.h"
#include "ItemDragDropOperation.generated.h"

UCLASS()
class STUDYPROJECT_API UItemDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Drag")
    ESlotContext SourceContext = ESlotContext::Inventory;

    UPROPERTY(BlueprintReadWrite, Category = "Drag")
    int32 SourceSlotIndex = -1;
};
