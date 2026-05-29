#include "MainMenuWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (NewGameButton)
    {
        NewGameButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleNewGame);
    }
    if (ContinueButton)
    {
        ContinueButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleContinue);
    }
    if (SettingsButton)
    {
        SettingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleSettings);
    }
    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleQuit);
    }
}

void UMainMenuWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (NewGameButton)
    {
        NewGameButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleNewGame);
    }
    if (ContinueButton)
    {
        ContinueButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleContinue);
    }
    if (SettingsButton)
    {
        SettingsButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleSettings);
    }
    if (QuitButton)
    {
        QuitButton->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleQuit);
    }
}

void UMainMenuWidget::HandleNewGame()
{
    UGameplayStatics::OpenLevel(this, GameLevelName);
}

void UMainMenuWidget::HandleContinue()
{
    UGameplayStatics::OpenLevel(this, GameLevelName);
}

void UMainMenuWidget::HandleSettings()
{
    // TODO: SettingsWidget Push
}

void UMainMenuWidget::HandleQuit()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
