#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "CompareTooltipWidget.generated.h"

UCLASS(Abstract)
class STUDYPROJECT_API UCompareTooltipWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetCompareData(const FItemData& NewItem, const FItemData& EquippedItem);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetPosition(FVector2D ScreenPos);

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnCompareDataSet(const FItemData& NewItem, const FItemData& EquippedItem,
                          int32 ATKDiff, int32 DEFDiff, int32 HPDiff);
};
