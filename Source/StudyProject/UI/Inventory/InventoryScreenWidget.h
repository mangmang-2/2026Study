#pragma once

#include "CoreMinimal.h"
#include "UI/Common/BaseGameWidget.h"
#include "InventoryScreenWidget.generated.h"

class UInventoryWidget;
class UEquipmentWidget;
class UCompareTooltipWidget;
class UTextBlock;

UCLASS(Abstract)
class STUDYPROJECT_API UInventoryScreenWidget : public UBaseGameWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void RefreshBottomBar();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowCompareTooltip(int32 InvSlotIndex);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnCloseBtnClicked();

protected:
    // UserWidget 인스턴스는 BindWidget 대신 NativeConstruct에서 GetWidgetFromName으로 수동 바인딩
    UPROPERTY() TObjectPtr<UInventoryWidget>      CachedInvWidget     = nullptr;
    UPROPERTY() TObjectPtr<UEquipmentWidget>      CachedEquipWidget   = nullptr;
    UPROPERTY() TObjectPtr<UCompareTooltipWidget> CachedCompareWidget = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> WeightText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GoldText   = nullptr;

    virtual void NativeConstruct() override;
};
