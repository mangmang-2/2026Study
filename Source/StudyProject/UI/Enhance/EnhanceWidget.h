#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "Data/SlotContext.h"
#include "EnhanceWidget.generated.h"

class UEnhanceComponent;
class UItemSlotWidget;
class UTextBlock;
class UButton;

UCLASS(Abstract)
class STUDYPROJECT_API UEnhanceWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Enhance")
    void BindToEnhance(UEnhanceComponent* InEnhanceComp);

    UFUNCTION(BlueprintCallable, Category = "Enhance")
    void OnTargetSlotDrop(int32 InvSlot);

    UFUNCTION(BlueprintCallable, Category = "Enhance")
    void OnMaterialSlotDrop(int32 InvSlot);

    UFUNCTION(BlueprintCallable, Category = "Enhance")
    void RefreshEnhanceInfo();

    UFUNCTION(BlueprintCallable, Category = "Enhance")
    void ShowEnhanceResult(bool bSuccess);

    UFUNCTION(BlueprintCallable, Category = "Enhance")
    void ClearSlots();

protected:
    // UserWidget 인스턴스는 BindWidget 대신 NativzzzzzzeConstruct에서 GetWidgetFromName으로 수동 바인딩
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedTargetSlotWidget   = nullptr;
    UPROPERTY(meta = (BindWidget)) 
    TObjectPtr<UTextBlock> LevelText          = nullptr;
    UPROPERTY(meta = (BindWidget)) 
    TObjectPtr<UTextBlock> SuccessRateText    = nullptr;
    UPROPERTY(meta = (BindWidget)) 
    TObjectPtr<UTextBlock> MaterialText       = nullptr;
    UPROPERTY(meta = (BindWidget)) 
    TObjectPtr<UTextBlock> GoldCostText       = nullptr;
    UPROPERTY(meta = (BindWidget)) 
    TObjectPtr<UTextBlock> ResultText         = nullptr;
    UPROPERTY(meta = (BindWidget))         
    TObjectPtr<UButton> EnhanceButton      = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Enhance")
    int32 TargetSlotIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Enhance")
    int32 MaterialSlotIndex = -1;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    TWeakObjectPtr<UEnhanceComponent> EnhanceComp;

    UFUNCTION() void HandleEnhanceButton();
    UFUNCTION() void HandleEnhanceResult(bool bSuccess, int32 NewLevel);
    UFUNCTION() void HandleTargetDrop(ESlotContext SourceContext, int32 FromSlot, int32 ToSlot);
};
