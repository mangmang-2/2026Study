#pragma once

#include "CoreMinimal.h"
#include "UI/Common/BaseGameWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;

UCLASS(Abstract)
class STUDYPROJECT_API UMainMenuWidget : public UBaseGameWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> NewGameButton  = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ContinueButton = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SettingsButton = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> QuitButton     = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Level")
    FName GameLevelName = TEXT("GameLevel");

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION() void HandleNewGame();
    UFUNCTION() void HandleContinue();
    UFUNCTION() void HandleSettings();
    UFUNCTION() void HandleQuit();
};
