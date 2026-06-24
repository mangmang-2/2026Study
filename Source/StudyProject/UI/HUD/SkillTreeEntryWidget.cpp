#include "SkillTreeEntryWidget.h"
#include "Skills/SkillDefinition.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"

UButton* USkillTreeEntryWidget::MakeSlotButton(const FString& Label)
{
    UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    T->SetText(FText::FromString(Label));
    T->SetJustification(ETextJustify::Center);
    Btn->AddChild(T);
    return Btn;
}

TSharedRef<SWidget> USkillTreeEntryWidget::RebuildWidget()
{
    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
        WidgetTree->RootWidget = Row;

        NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
        FSlateFontInfo F = NameText->GetFont();
        F.Size = 16.f;
        NameText->SetFont(F);
        if (UHorizontalBoxSlot* NS = Row->AddChildToHorizontalBox(NameText))
        {
            NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            NS->SetVerticalAlignment(VAlign_Center);
            NS->SetPadding(FMargin(8.f, 4.f));
        }

        const TCHAR* Labels[3] = { TEXT("Z"), TEXT("X"), TEXT("C") };
        for (int32 i = 0; i < 3; ++i)
        {
            UButton* Btn = MakeSlotButton(Labels[i]);
            if (i == 0) { Btn->OnClicked.AddDynamic(this, &USkillTreeEntryWidget::OnSlot0); }
            else if (i == 1) { Btn->OnClicked.AddDynamic(this, &USkillTreeEntryWidget::OnSlot1); }
            else { Btn->OnClicked.AddDynamic(this, &USkillTreeEntryWidget::OnSlot2); }

            USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
            Sz->SetWidthOverride(40.f);
            Sz->AddChild(Btn);
            if (UHorizontalBoxSlot* BS = Row->AddChildToHorizontalBox(Sz))
            {
                BS->SetPadding(FMargin(2.f, 2.f));
            }
        }
    }
    return Super::RebuildWidget();
}

void USkillTreeEntryWidget::InitEntry(int32 InPoolIndex, USkillDefinition* InSkill)
{
    PoolIndex = InPoolIndex;
    if (NameText != nullptr)
    {
        const FText N = (InSkill != nullptr && InSkill->SkillName.IsEmpty() == false)
            ? InSkill->SkillName
            : FText::FromString(InSkill != nullptr ? InSkill->GetName() : TEXT("None"));
        NameText->SetText(N);
    }
}

void USkillTreeEntryWidget::OnSlot0()
{
    OnAssignRequested.Broadcast(PoolIndex, 0);
}

void USkillTreeEntryWidget::OnSlot1()
{
    OnAssignRequested.Broadcast(PoolIndex, 1);
}

void USkillTreeEntryWidget::OnSlot2()
{
    OnAssignRequested.Broadcast(PoolIndex, 2);
}
