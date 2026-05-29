#pragma once

#include "CoreMinimal.h"
#include "UI/Common/BaseGameWidget.h"
#include "ShopScreenWidget.generated.h"

class UShopWidget;
class UInventoryWidget;
class UTextBlock;

UCLASS(Abstract)
class STUDYPROJECT_API UShopScreenWidget : public UBaseGameWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void SetShopID(int32 ShopID);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    void RefreshGold();

    UFUNCTION(BlueprintCallable, Category = "Shop")
    void OnCloseBtnClicked();

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UShopWidget> ShopPanel = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UInventoryWidget> InvPanel  = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> GoldText  = nullptr;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION() void HandleBuy(int32 ItemID, int32 Quantity);
    UFUNCTION() void HandleSell(int32 InvSlot, int32 Quantity);
};
