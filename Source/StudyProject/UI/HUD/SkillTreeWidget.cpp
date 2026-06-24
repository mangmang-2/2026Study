#include "SkillTreeWidget.h"
#include "SkillTreeEntryWidget.h"
#include "Skills/SkillManagerComponent.h"
#include "Skills/SkillDefinition.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> USkillTreeWidget::RebuildWidget()
{
    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
        WidgetTree->RootWidget = Canvas;

        UBorder* Bg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Bg"));
        Bg->SetBrushColor(FLinearColor(0.05f, 0.06f, 0.10f, 0.96f));
        Bg->SetPadding(FMargin(12.f));

        USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Sz"));
        Sz->SetWidthOverride(440.f);
        Sz->SetHeightOverride(420.f);
        Bg->AddChild(Sz);

        UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
        Sz->AddChild(Box);

        UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
        Title->SetText(FText::FromString(TEXT("스킬 배정 (Z / X / C)")));
        FSlateFontInfo TF = Title->GetFont();
        TF.Size = 20.f;
        Title->SetFont(TF);
        Box->AddChildToVerticalBox(Title);

        SlotsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotsText"));
        FSlateFontInfo SF = SlotsText->GetFont();
        SF.Size = 13.f;
        SlotsText->SetFont(SF);
        SlotsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.8f, 0.6f)));
        if (UVerticalBoxSlot* SS = Box->AddChildToVerticalBox(SlotsText))
        {
            SS->SetPadding(FMargin(0.f, 4.f, 0.f, 8.f));
        }

        ListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ListBox"));
        if (UVerticalBoxSlot* LS = Box->AddChildToVerticalBox(ListBox))
        {
            LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }

        if (UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Bg))
        {
            CS->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
            CS->SetAlignment(FVector2D(0.5f, 0.5f));
            CS->SetAutoSize(true);
        }
    }
    return Super::RebuildWidget();
}

void USkillTreeWidget::InitTree(USkillManagerComponent* InComp)
{
    if (SkillComp.IsValid())
    {
        SkillComp->OnSlotsChanged.RemoveDynamic(this, &USkillTreeWidget::HandleSlotsChanged);
    }

    SkillComp = InComp;

    if (SkillComp.IsValid())
    {
        SkillComp->OnSlotsChanged.AddDynamic(this, &USkillTreeWidget::HandleSlotsChanged);
    }

    RefreshList();
}

void USkillTreeWidget::RefreshList()
{
    if (ListBox == nullptr || SkillComp.IsValid() == false)
    {
        return;
    }

    ListBox->ClearChildren();

    TArray<USkillDefinition*> Pool = SkillComp->GetSkillPool();
    for (int32 i = 0; i < Pool.Num(); ++i)
    {
        USkillTreeEntryWidget* Entry = CreateWidget<USkillTreeEntryWidget>(GetOwningPlayer(), USkillTreeEntryWidget::StaticClass());
        if (Entry != nullptr)
        {
            Entry->InitEntry(i, Pool[i]);
            Entry->OnAssignRequested.AddDynamic(this, &USkillTreeWidget::HandleAssign);
            ListBox->AddChild(Entry);
        }
    }

    HandleSlotsChanged();
}

void USkillTreeWidget::HandleAssign(int32 PoolIndex, int32 InSlot)
{
    if (SkillComp.IsValid() == false)
    {
        return;
    }

    TArray<USkillDefinition*> Pool = SkillComp->GetSkillPool();
    if (Pool.IsValidIndex(PoolIndex))
    {
        SkillComp->AssignSkill(Pool[PoolIndex], InSlot);
    }
}

void USkillTreeWidget::HandleSlotsChanged()
{
    if (SlotsText == nullptr || SkillComp.IsValid() == false)
    {
        return;
    }

    auto Name = [](USkillDefinition* S) -> FString
    {
        if (S == nullptr) { return TEXT("(빈 슬롯)"); }
        return S->SkillName.IsEmpty() ? S->GetName() : S->SkillName.ToString();
    };

    const FString Txt = FString::Printf(TEXT("Z: %s    X: %s    C: %s"),
        *Name(SkillComp->GetSlotSkill(0)),
        *Name(SkillComp->GetSlotSkill(1)),
        *Name(SkillComp->GetSlotSkill(2)));
    SlotsText->SetText(FText::FromString(Txt));
}
