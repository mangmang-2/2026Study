#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionPromptWidget.generated.h"

class UTextBlock;

UCLASS(Abstract)
class STUDYPROJECT_API UInteractionPromptWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetPromptText(const FText& Text);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void Show();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void Hide();

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PromptText = nullptr;
};
