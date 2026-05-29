#pragma once

#include "CoreMinimal.h"
#include "UI/Common/BaseGameWidget.h"
#include "SettingsWidget.generated.h"

class USlider;
class UComboBoxString;
class UButton;

UCLASS(Abstract)
class STUDYPROJECT_API USettingsWidget : public UBaseGameWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void OnApplyBtnClicked();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void OnCloseBtnClicked();

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> MasterVolumeSlider = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> BGMVolumeSlider    = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> SFXVolumeSlider    = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> SensitivitySlider  = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> ResolutionCombo    = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> WindowModeCombo    = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ApplyButton        = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CloseButton        = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Settings") float MasterVolume    = 1.f;
    UPROPERTY(BlueprintReadOnly, Category = "Settings") float SFXVolume       = 1.f;
    UPROPERTY(BlueprintReadOnly, Category = "Settings") float BGMVolume       = 1.f;
    UPROPERTY(BlueprintReadOnly, Category = "Settings") int32 ResolutionIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Settings") int32 WindowModeIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Settings") float MouseSensitivity= 1.f;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION() void HandleMasterVolumeChanged(float Value);
    UFUNCTION() void HandleBGMVolumeChanged(float Value);
    UFUNCTION() void HandleSFXVolumeChanged(float Value);
    UFUNCTION() void HandleSensitivityChanged(float Value);
    UFUNCTION() void HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
};
