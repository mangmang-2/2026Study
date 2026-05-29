#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/GameData.h"
#include "DialogueWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UButton;
class UDataTable;

UCLASS(Abstract)
class STUDYPROJECT_API UDialogueWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // DialogueID: DT_DialogueData 키, Interactor: 상호작용을 시작한 액터
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void StartDialogue(int32 InDialogueID, AActor* InInteractor);

    // 다음 노드로 이동 (선택지 없는 경우 "다음" 버튼)
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void AdvanceNode();

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void SelectChoice(int32 ChoiceIndex);

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void CloseDialogue();

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
    TObjectPtr<UDataTable> DialogueTable;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SpeakerNameText = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DialogueBodyText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> ChoiceBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> NextButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Choice0Button = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Choice1Button = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Choice2Button = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Choice3Button = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Choice0Text = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Choice1Text = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Choice2Text = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Choice3Text = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
    void OnDialogueStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
    void OnDialogueClosed();

    // C++ 기본 구현 — 최대 4개 고정 버튼 표시/숨김
    UFUNCTION(BlueprintNativeEvent, Category = "Dialogue")
    void OnChoicesReady(const TArray<FDialogueChoice>& Choices);

    virtual void NativeOnInitialized() override;

private:
    void ShowNode(const FDialogueRow* Row);
    void HandleChoiceAction(EDialogueAction Action, int32 NextNode);

    UFUNCTION()
    void OnChoice0Clicked();

    UFUNCTION()
    void OnChoice1Clicked();

    UFUNCTION()
    void OnChoice2Clicked();

    UFUNCTION()
    void OnChoice3Clicked();

    int32 DialogueID    = -1;
    int32 CurrentNode   = 0;
    UPROPERTY() TObjectPtr<AActor> 
    Interactor = nullptr;
};
