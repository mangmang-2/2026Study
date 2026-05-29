#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(Abstract)
class STUDYPROJECT_API UHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateHP(float Current, float Max);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateSP(float Current, float Max);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateLevel(int32 Level);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void FlashHPBar();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void FlashSPBar();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowAcquisitionNotify(UTexture2D* Icon, const FText& ItemName);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateQuestTracker(const FText& QuestText);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowLockOnHP(const FText& Name, float HP, float Max);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void HideLockOnHP();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateComboCount(int32 Count);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void HideComboCount();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowFinisherPrompt();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void HideFinisherPrompt();

protected:
    UPROPERTY(meta = (BindWidget))         TObjectPtr<UProgressBar> HPBar         = nullptr;
    UPROPERTY(meta = (BindWidget))         TObjectPtr<UProgressBar> SPBar         = nullptr;
    UPROPERTY(meta = (BindWidget))         TObjectPtr<UTextBlock> HPText         = nullptr;
    UPROPERTY(meta = (BindWidget))         TObjectPtr<UTextBlock> SPText         = nullptr;
    UPROPERTY(meta = (BindWidget))         TObjectPtr<UTextBlock> LevelText      = nullptr;
    UPROPERTY(meta = (BindWidget))         TObjectPtr<UTextBlock> ComboText      = nullptr;
    UPROPERTY(meta = (BindWidget))         TObjectPtr<UTextBlock> QuestText      = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> LockOnHPBar    = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LockOnNameText = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlayHPFlashAnimation();

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlaySPFlashAnimation();

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlayComboScaleAnimation();

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlayComboFadeOut();

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlayFinisherPulse();
};
