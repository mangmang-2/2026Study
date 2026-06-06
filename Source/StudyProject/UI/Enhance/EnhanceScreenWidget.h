#pragma once

#include "CoreMinimal.h"
#include "UI/Common/BaseGameWidget.h"
#include "EnhanceScreenWidget.generated.h"

class UEnhanceWidget;
class UInventoryWidget;

UCLASS(Abstract)
class STUDYPROJECT_API UEnhanceScreenWidget : public UBaseGameWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnCloseBtnClicked();

protected:
    // UserWidget 인스턴스는 BindWidget 대신 NativeConstruct에서 GetWidgetFromName으로 수동 바인딩
    UPROPERTY() TObjectPtr<UEnhanceWidget>   CachedEnhancePanel = nullptr;
    UPROPERTY() TObjectPtr<UInventoryWidget> CachedInvPanel     = nullptr;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    // 인벤 우클릭 → 강화 대상 지정
    UFUNCTION()
    void HandleInvItemSelected(int32 SlotIndex);
};
