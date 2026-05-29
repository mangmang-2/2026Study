#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHPBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(Abstract)
class STUDYPROJECT_API UEnemyHPBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateHP(float Current, float Max);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowForDuration(float Seconds);

protected:
    UPROPERTY(meta = (BindWidgetOptional)) 
    TObjectPtr<UProgressBar> HPBar    = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) 
    TObjectPtr<UTextBlock> NameText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) 
    TObjectPtr<UTextBlock> HPText   = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlayFadeOutAfterDelay(float Delay);
};
