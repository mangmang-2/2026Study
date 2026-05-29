#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FinisherPromptWidget.generated.h"

UCLASS(Abstract)
class STUDYPROJECT_API UFinisherPromptWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void Show();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void Hide();

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnShow();

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnHide();
};
