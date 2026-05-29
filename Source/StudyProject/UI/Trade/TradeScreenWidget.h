#pragma once

#include "CoreMinimal.h"
#include "UI/Common/BaseGameWidget.h"
#include "TradeScreenWidget.generated.h"

class UTradeWidget;
class UInventoryWidget;
class UTradeComponent;

UCLASS(Abstract)
class STUDYPROJECT_API UTradeScreenWidget : public UBaseGameWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Trade")
    void OpenTrade(ACharacter* TargetPlayer);

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void BindToTradeComponent(UTradeComponent* InTradeComp);

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void OnCloseBtnClicked();

protected:
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTradeWidget> TradePanel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UInventoryWidget> InvPanel   = nullptr;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
};
