#include "SkillTreeWidget.h"
#include "SkillTreeEntryWidget.h"
#include "Skills/SkillManagerComponent.h"
#include "Skills/SkillDefinition.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Font.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "SkillUIStyle.h"

namespace SkillTreeStyle
{
    static const FLinearColor Header  (0.f, 0.f, 0.f, 0.45f);

    static FSlateFontInfo Font(float Size) { return SkillUI::Font(Size); }
}

TSharedRef<SWidget> USkillTreeWidget::RebuildWidget()
{
    using namespace SkillTreeStyle;

    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
        WidgetTree->RootWidget = Canvas;

        USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Sz"));
        Sz->SetWidthOverride(460.f);
        Sz->SetHeightOverride(440.f);

        UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
        Sz->AddChild(Box);

        UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
        Title->SetText(FText::FromString(TEXT("SKILLS  —  Z / X / C")));
        Title->SetFont(SkillUI::Font(26.f));
        Title->SetColorAndOpacity(FSlateColor(SkillUI::TextMain()));
        Box->AddChildToVerticalBox(Title);

        SlotsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotsText"));
        SlotsText->SetFont(SkillUI::Font(14.f));
        SlotsText->SetColorAndOpacity(FSlateColor(SkillUI::TextDim()));
        if (UVerticalBoxSlot* SS = Box->AddChildToVerticalBox(SlotsText))
        {
            SS->SetPadding(FMargin(0.f, 6.f, 0.f, 10.f));
        }

        ListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ListBox"));
        if (UVerticalBoxSlot* LS = Box->AddChildToVerticalBox(ListBox))
        {
            LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }

        // AGIS 패널(블러 + 검정 반투명 + 회색 외곽선)로 감쌈
        UWidget* Panel = SkillUI::MakePanel(WidgetTree, Sz, FMargin(18.f));
        if (UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Panel))
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
    using namespace SkillTreeStyle;

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

            // 행 배경: 검정 반투명 + 회색 외곽선(AGIS 톤)
            UBorder* RowBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
            SkillUI::ApplyOutline(RowBg, SkillUI::OutlineCol());
            RowBg->SetPadding(FMargin(8.f, 5.f));
            RowBg->AddChild(Entry);
            if (UScrollBoxSlot* RS = Cast<UScrollBoxSlot>(ListBox->AddChild(RowBg)))
            {
                RS->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
            }
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
        if (S == nullptr) { return TEXT("- "); }
        return S->SkillName.IsEmpty() ? S->GetName() : S->SkillName.ToString();
    };

    const FString Txt = FString::Printf(TEXT("Z: %s     X: %s     C: %s"),
        *Name(SkillComp->GetSlotSkill(0)),
        *Name(SkillComp->GetSlotSkill(1)),
        *Name(SkillComp->GetSlotSkill(2)));
    SlotsText->SetText(FText::FromString(Txt));
}
