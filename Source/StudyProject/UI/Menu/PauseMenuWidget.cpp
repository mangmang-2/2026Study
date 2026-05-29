#include "PauseMenuWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (ResumeButton)
    {
        ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleResume);
    }
    if (SettingsButton)
    {
        SettingsButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleSettings);
    }
    if (MainMenuButton)
    {
        MainMenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleMainMenu);
    }
    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleQuit);
    }
}

void UPauseMenuWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (ResumeButton)
    {
        ResumeButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleResume);
    }
    if (SettingsButton)
    {
        SettingsButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleSettings);
    }
    if (MainMenuButton)
    {
        MainMenuButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleMainMenu);
    }
    if (QuitButton)
    {
        QuitButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleQuit);
    }
}

void UPauseMenuWidget::HandleResume()
{
    DeactivateWidget();
    UGameplayStatics::SetGamePaused(this, false);
}

void UPauseMenuWidget::HandleSettings()
{
    // TODO: SettingsWidget Push
}

void UPauseMenuWidget::HandleMainMenu()
{
    UGameplayStatics::OpenLevel(this, TEXT("MainMenu"));
}

void UPauseMenuWidget::HandleQuit()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
