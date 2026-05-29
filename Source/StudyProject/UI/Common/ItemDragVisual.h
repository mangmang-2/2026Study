#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "ItemDragVisual.generated.h"

UCLASS(Abstract)
class STUDYPROJECT_API UItemDragVisual : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void SetItemData(const FItemData& Data);
};
