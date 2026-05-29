#include "DialogueWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "Engine/DataTable.h"

void UDialogueWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (NextButton)
    {
        NextButton->OnClicked.AddDynamic(this, &UDialogueWidget::AdvanceNode);
    }
    if (Choice0Button)
    {
        Choice0Button->OnClicked.AddDynamic(this, &UDialogueWidget::OnChoice0Clicked);
    }
    if (Choice1Button)
    {
        Choice1Button->OnClicked.AddDynamic(this, &UDialogueWidget::OnChoice1Clicked);
    }
    if (Choice2Button)
    {
        Choice2Button->OnClicked.AddDynamic(this, &UDialogueWidget::OnChoice2Clicked);
    }
    if (Choice3Button)
    {
        Choice3Button->OnClicked.AddDynamic(this, &UDialogueWidget::OnChoice3Clicked);
    }
}

void UDialogueWidget::StartDialogue(int32 InDialogueID, AActor* InInteractor)
{
    if (!DialogueTable) return;

    DialogueID  = InDialogueID;
    Interactor  = InInteractor;
    CurrentNode = 0;

    const FString RowKey = FString::FromInt(DialogueID * 1000 + CurrentNode);
    const FDialogueRow* Row = DialogueTable->FindRow<FDialogueRow>(FName(*RowKey), TEXT("DialogueWidget"));
    if (!Row) return;

    ShowNode(Row);
    SetVisibility(ESlateVisibility::Visible);
    OnDialogueStarted();
}

void UDialogueWidget::AdvanceNode()
{
    if (!DialogueTable) return;

    ++CurrentNode;
    const FString RowKey = FString::FromInt(DialogueID * 1000 + CurrentNode);
    const FDialogueRow* Row = DialogueTable->FindRow<FDialogueRow>(FName(*RowKey), TEXT("DialogueWidget"));

    if (!Row)
    {
        CloseDialogue();
        return;
    }

    ShowNode(Row);
}

void UDialogueWidget::SelectChoice(int32 ChoiceIndex)
{
    if (!DialogueTable) return;

    const FString RowKey = FString::FromInt(DialogueID * 1000 + CurrentNode);
    const FDialogueRow* Row = DialogueTable->FindRow<FDialogueRow>(FName(*RowKey), TEXT("DialogueWidget"));
    if (!Row || !Row->Choices.IsValidIndex(ChoiceIndex)) return;

    const FDialogueChoice& Choice = Row->Choices[ChoiceIndex];
    HandleChoiceAction(Choice.Action, Choice.NextNode);
}

void UDialogueWidget::CloseDialogue()
{
    SetVisibility(ESlateVisibility::Collapsed);
    OnDialogueClosed();
}

void UDialogueWidget::ShowNode(const FDialogueRow* Row)
{
    if (SpeakerNameText)  SpeakerNameText->SetText(Row->SpeakerName);
    if (DialogueBodyText) DialogueBodyText->SetText(Row->DialogueText);

    const bool bHasChoices = Row->bHasChoices && Row->Choices.Num() > 0;

    if (NextButton)   NextButton->SetVisibility(bHasChoices ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (ChoiceBox)    ChoiceBox->SetVisibility(bHasChoices ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    if (bHasChoices)
        OnChoicesReady(Row->Choices);
}

void UDialogueWidget::OnChoicesReady_Implementation(const TArray<FDialogueChoice>& Choices)
{
    auto SetSlot = [](UButton* Btn, UTextBlock* Txt, const FText& Label, bool bShow)
    {
        if (Btn == nullptr)
        {
            return;
        }
        Btn->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (Txt && bShow)
        {
            Txt->SetText(Label);
        }
    };

    SetSlot(Choice0Button, Choice0Text, Choices.IsValidIndex(0) ? Choices[0].ChoiceText : FText::GetEmpty(), Choices.IsValidIndex(0));
    SetSlot(Choice1Button, Choice1Text, Choices.IsValidIndex(1) ? Choices[1].ChoiceText : FText::GetEmpty(), Choices.IsValidIndex(1));
    SetSlot(Choice2Button, Choice2Text, Choices.IsValidIndex(2) ? Choices[2].ChoiceText : FText::GetEmpty(), Choices.IsValidIndex(2));
    SetSlot(Choice3Button, Choice3Text, Choices.IsValidIndex(3) ? Choices[3].ChoiceText : FText::GetEmpty(), Choices.IsValidIndex(3));
}

void UDialogueWidget::OnChoice0Clicked()
{
    SelectChoice(0);
}

void UDialogueWidget::OnChoice1Clicked()
{
    SelectChoice(1);
}

void UDialogueWidget::OnChoice2Clicked()
{
    SelectChoice(2);
}

void UDialogueWidget::OnChoice3Clicked()
{
    SelectChoice(3);
}

void UDialogueWidget::HandleChoiceAction(EDialogueAction Action, int32 NextNode)
{
    switch (Action)
    {
    case EDialogueAction::NextNode:
        CurrentNode = NextNode;
        {
            const FString RowKey = FString::FromInt(DialogueID * 1000 + CurrentNode);
            const FDialogueRow* Row = DialogueTable->FindRow<FDialogueRow>(FName(*RowKey), TEXT("DialogueWidget"));
            if (Row) ShowNode(Row);
            else     CloseDialogue();
        }
        break;

    case EDialogueAction::OpenShop:
        // NPCShop BP 이벤트로 상점 오픈 — Interactor(PlayerCharacter)에 알림
        CloseDialogue();
        break;

    case EDialogueAction::AcceptQuest:
    case EDialogueAction::CompleteQuest:
        // 3단계 퀘스트 시스템에서 처리
        AdvanceNode();
        break;

    case EDialogueAction::Exit:
        CloseDialogue();
        break;
    }
}
