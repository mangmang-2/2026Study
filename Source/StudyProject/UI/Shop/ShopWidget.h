#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "ShopWidget.generated.h"

class UScrollBox;
class UItemSlotWidget;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopBuy,  int32, ItemID, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopSell, int32, InvSlot, int32, Quantity);

UCLASS(Abstract)
class STUDYPROJECT_API UShopWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void InitShop(int32 ShopID);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    void RefreshShopList();

    UPROPERTY(BlueprintAssignable, Category = "Shop") FOnShopBuy  OnBuyClicked;
    UPROPERTY(BlueprintAssignable, Category = "Shop") FOnShopSell OnSellClicked;

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UScrollBox> ShopList = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> GoldText = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    TSubclassOf<UItemSlotWidget> ShopSlotClass;

private:
    int32 CurrentShopID = 0;

    UFUNCTION() void HandleShopItemClicked(int32 SlotIndex);
};
