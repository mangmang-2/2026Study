#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "Data/SlotContext.h"
#include "ItemSlotWidget.generated.h"

class UItemIconWidget;
class UTextBlock;
class UProgressBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotHovered,      int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotRightClicked, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSlotDrop, ESlotContext, SourceContext, int32, FromSlot, int32, ToSlot);

UCLASS(Abstract)
class STUDYPROJECT_API UItemSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Slot")
    void SetItemData(const FItemData& Data, int32 InQuantity, int32 InEnhanceLevel);

    UFUNCTION(BlueprintCallable, Category = "Slot")
    void ClearSlot();

    UFUNCTION(BlueprintCallable, Category = "Slot")
    void SetCooldownPercent(float Percent);

    UPROPERTY(BlueprintReadOnly, Category = "Slot")
    int32 SlotIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Slot")
    ESlotContext SlotContext = ESlotContext::Inventory;

    UPROPERTY(BlueprintReadOnly, Category = "Slot")
    FItemData CachedItemData;

    UPROPERTY(BlueprintReadOnly, Category = "Slot")
    int32 CachedQuantity = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Slot")
    int32 CachedEnhanceLevel = 0;

    UPROPERTY(BlueprintAssignable, Category = "Slot")
    FOnSlotHovered OnSlotHovered;

    UPROPERTY(BlueprintAssignable, Category = "Slot")
    FOnSlotRightClicked OnSlotRightClicked;

    UPROPERTY(BlueprintAssignable, Category = "Slot")
    FOnSlotDrop OnSlotDrop;

protected:
    // UserWidget 인스턴스는 BindWidget 대신 NativeConstruct에서 수동 바인딩(이름 충돌 방지)
    UPROPERTY()
    TObjectPtr<UItemIconWidget> CachedIconWidget = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> QuantityText = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> EnhanceText  = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> CooldownBar  = nullptr;

    virtual void NativeConstruct() override;
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual void   NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual void   NativeOnMouseLeave(const FPointerEvent& Event) override;
    virtual void   NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& OutOperation) override;
    virtual bool   NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;

private:
    bool bHasItem = false;
};
