#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ComboCounterWidget.generated.h"

UCLASS(Abstract)
class STUDYPROJECT_API UComboCounterWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetCount(int32 Count);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ResetCount();

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnCountSet(int32 Count);

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlayPulseScale();

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlayFadeOut();
};
