#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/TradeComponent.h"
#include "TradeWidget.generated.h"

class UScrollBox;
class UItemSlotWidget;
class UTextBlock;
class UButton;

UCLASS(Abstract)
class STUDYPROJECT_API UTradeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Trade")
    void BindToTrade(UTradeComponent* InTradeComp);

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void RefreshTrade();

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void RefreshOtherPanel();

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void RefreshStatus();

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void OnTradeCompleted();

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void OnTradeCancelled(const FString& Reason);

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UScrollBox> MyOfferList      = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UScrollBox> PartnerOfferList = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ConfirmButton    = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CancelButton     = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> StatusText       = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Trade")
    TSubclassOf<UItemSlotWidget> TradeSlotClass;

    UPROPERTY(BlueprintReadOnly, Category = "Trade")
    TArray<int32> MyTradeSlots;

    UPROPERTY(BlueprintReadOnly, Category = "Trade")
    int32 MyTradeGold = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Trade")
    bool bMyConfirmed = false;

    UPROPERTY(BlueprintReadOnly, Category = "Trade")
    bool bOtherConfirmed = false;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    TWeakObjectPtr<UTradeComponent> TradeComp;

    UFUNCTION() void HandleConfirm();
    UFUNCTION() void HandleCancel();
    UFUNCTION() void HandleTradeUpdated();
    UFUNCTION() void HandleTradeResult(bool bSuccess);
};
