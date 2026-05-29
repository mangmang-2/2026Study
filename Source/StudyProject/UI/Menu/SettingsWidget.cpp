#include "SettingsWidget.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "GameFramework/GameUserSettings.h"

void USettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ApplyButton)
    {
        ApplyButton->OnClicked.AddDynamic(this, &USettingsWidget::OnApplyBtnClicked);
    }
    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &USettingsWidget::OnCloseBtnClicked);
    }

    if (MasterVolumeSlider)
    {
        MasterVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleMasterVolumeChanged);
    }
    if (BGMVolumeSlider)
    {
        BGMVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleBGMVolumeChanged);
    }
    if (SFXVolumeSlider)
    {
        SFXVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleSFXVolumeChanged);
    }
    if (SensitivitySlider)
    {
        SensitivitySlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleSensitivityChanged);
    }
    if (ResolutionCombo)
    {
        ResolutionCombo->OnSelectionChanged.AddDynamic(this, &USettingsWidget::HandleResolutionChanged);
    }
    if (WindowModeCombo)
    {
        WindowModeCombo->OnSelectionChanged.AddDynamic(this, &USettingsWidget::HandleWindowModeChanged);
    }

    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (Settings == nullptr)
    {
        return;
    }

    if (MasterVolumeSlider)
    {
        MasterVolumeSlider->SetValue(MasterVolume);
    }
    if (BGMVolumeSlider)
    {
        BGMVolumeSlider->SetValue(BGMVolume);
    }
    if (SFXVolumeSlider)
    {
        SFXVolumeSlider->SetValue(SFXVolume);
    }

    EWindowMode::Type WM = Settings->GetFullscreenMode();
    WindowModeIndex = (WM == EWindowMode::Fullscreen) ? 0 : (WM == EWindowMode::WindowedFullscreen) ? 1 : 2;
}

void USettingsWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (ApplyButton)
    {
        ApplyButton->OnClicked.RemoveDynamic(this, &USettingsWidget::OnApplyBtnClicked);
    }
    if (CloseButton)
    {
        CloseButton->OnClicked.RemoveDynamic(this, &USettingsWidget::OnCloseBtnClicked);
    }
    if (MasterVolumeSlider)
    {
        MasterVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsWidget::HandleMasterVolumeChanged);
    }
    if (BGMVolumeSlider)
    {
        BGMVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsWidget::HandleBGMVolumeChanged);
    }
    if (SFXVolumeSlider)
    {
        SFXVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsWidget::HandleSFXVolumeChanged);
    }
    if (SensitivitySlider)
    {
        SensitivitySlider->OnValueChanged.RemoveDynamic(this, &USettingsWidget::HandleSensitivityChanged);
    }
    if (ResolutionCombo)
    {
        ResolutionCombo->OnSelectionChanged.RemoveDynamic(this, &USettingsWidget::HandleResolutionChanged);
    }
    if (WindowModeCombo)
    {
        WindowModeCombo->OnSelectionChanged.RemoveDynamic(this, &USettingsWidget::HandleWindowModeChanged);
    }
}

void USettingsWidget::OnApplyBtnClicked()
{
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (Settings == nullptr)
    {
        return;
    }

    EWindowMode::Type Modes[] = { EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed };
    if (WindowModeIndex >= 0 && WindowModeIndex < 3)
    {
        Settings->SetFullscreenMode(Modes[WindowModeIndex]);
    }

    Settings->ApplySettings(false);
}

void USettingsWidget::OnCloseBtnClicked()
{
    DeactivateWidget();
}

void USettingsWidget::HandleMasterVolumeChanged(float Value)
{
    MasterVolume = Value;
}

void USettingsWidget::HandleBGMVolumeChanged(float Value)
{
    BGMVolume = Value;
}

void USettingsWidget::HandleSFXVolumeChanged(float Value)
{
    SFXVolume = Value;
}

void USettingsWidget::HandleSensitivityChanged(float Value)
{
    MouseSensitivity = Value;
}

void USettingsWidget::HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (ResolutionCombo != nullptr)
    {
        ResolutionIndex = ResolutionCombo->FindOptionIndex(SelectedItem);
    }
}

void USettingsWidget::HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (WindowModeCombo != nullptr)
    {
        WindowModeIndex = WindowModeCombo->FindOptionIndex(SelectedItem);
    }
}
