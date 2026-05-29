#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryComponent.h"
#include "InventoryGridWidget.generated.h"

class UUniformGridPanel;
class UItemSlotWidget;

UCLASS(Abstract)
class STUDYPROJECT_API UInventoryGridWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void InitGrid(int32 SlotCount);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RefreshGrid(const TArray<FInventorySlot>& Slots);

    UItemSlotWidget* GetSlotWidget(int32 Index) const;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    TSubclassOf<UItemSlotWidget> SlotWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    int32 ColumnCount = 5;

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UUniformGridPanel> GridPanel = nullptr;

private:
    UPROPERTY()
    TArray<UItemSlotWidget*> SlotWidgets;
};
