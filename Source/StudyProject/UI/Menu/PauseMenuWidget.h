#pragma once

#include "CoreMinimal.h"
#include "UI/Common/BaseGameWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;

UCLASS(Abstract)
class STUDYPROJECT_API UPauseMenuWidget : public UBaseGameWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ResumeButton   = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SettingsButton = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> MainMenuButton = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> QuitButton     = nullptr;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION() void HandleResume();
    UFUNCTION() void HandleSettings();
    UFUNCTION() void HandleMainMenu();
    UFUNCTION() void HandleQuit();
};
