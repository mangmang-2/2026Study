#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConfirmPopupWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmed);

UCLASS(Abstract)
class STUDYPROJECT_API UConfirmPopupWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetMessage(const FText& Title, const FText& Body);

    UPROPERTY(BlueprintAssignable, Category = "UI")
    FOnConfirmed OnConfirmed;

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ConfirmBtn = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CancelBtn  = nullptr;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnMessageSet(const FText& Title, const FText& Body);

private:
    UFUNCTION() void HandleConfirmClicked();
    UFUNCTION() void HandleCancelClicked();
};
