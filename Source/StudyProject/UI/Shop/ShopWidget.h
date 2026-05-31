#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "Data/SlotContext.h"
#include "ShopWidget.generated.h"

class UScrollBox;
class UItemSlotWidget;
class UTextBlock;
class UShopComponent;

UCLASS(Abstract)
class STUDYPROJECT_API UShopWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 서버 권위 ShopComponent 연결(목록은 컴포넌트에서 읽음)
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void SetShopComponent(UShopComponent* InShopComp);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    void RefreshShopList();

protected:
    UPROPERTY(meta = (BindWidget))         TObjectPtr<UScrollBox> ShopList    = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UScrollBox> BuybackList = nullptr;   // 되사기 목록(있으면 표시)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GoldText    = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    TSubclassOf<UItemSlotWidget> ShopSlotClass;

    // 인벤 아이템을 상점 패널에 드롭하면 판매
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
    TWeakObjectPtr<UShopComponent> ShopComp;

    UFUNCTION() void HandleShopItemClicked(int32 SlotIndex);   // 상점 아이템 우클릭 → 구매
    UFUNCTION() void HandleBuybackClicked(int32 SlotIndex);    // 되사기 아이템 우클릭 → 재구매
    UFUNCTION() void HandleShopSlotDrop(ESlotContext SourceContext, int32 FromSlot, int32 ToSlot);
};
