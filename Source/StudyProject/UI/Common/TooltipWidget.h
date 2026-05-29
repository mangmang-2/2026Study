#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "TooltipWidget.generated.h"

UCLASS(Abstract)
class STUDYPROJECT_API UTooltipWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetItemData(const FItemData& Data);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetPosition(FVector2D ScreenPos);

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnItemDataSet(const FItemData& Data);
};
