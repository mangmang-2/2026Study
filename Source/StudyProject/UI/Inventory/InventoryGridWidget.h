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

    // SourceIndices[i] = Slots[i]가 가리키는 실제 인벤토리 슬롯 인덱스(필터/압축 표시 시 매핑용).
    // 비우면 그리드 위치 = 인벤 인덱스(All 표시)로 간주.
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RefreshGrid(const TArray<FInventorySlot>& Slots, const TArray<int32>& SourceIndices);

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
