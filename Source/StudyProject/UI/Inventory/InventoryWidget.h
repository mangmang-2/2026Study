#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "Data/SlotContext.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;
class UInventoryGridWidget;
class UTooltipWidget;
class UContextMenuWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSelected, int32, SlotIndex);

UCLASS(Abstract)
class STUDYPROJECT_API UInventoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void BindToInventory(UInventoryComponent* InvComp);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RefreshInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void FilterByType(EItemType Type);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SortItems(ESortMode Mode);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ShowTooltip(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void HideTooltip();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ShowContextMenu(int32 SlotIndex);

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnItemSelected OnItemSelected;

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UInventoryGridWidget> GridWidget  = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTooltipWidget> Tooltip     = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UContextMenuWidget> ContextMenu = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    ESlotContext Context = ESlotContext::Inventory;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    EItemType CurrentFilter = EItemType::All;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    ESortMode CurrentSort = ESortMode::ByRarity;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    TWeakObjectPtr<UInventoryComponent> BoundInventory;

    UFUNCTION() void HandleSlotHovered(int32 SlotIndex);
    UFUNCTION() void HandleSlotRightClicked(int32 SlotIndex);
    UFUNCTION() void HandleSlotDrop(int32 FromSlot, int32 ToSlot);
    UFUNCTION() void HandleInventoryChanged();
};
