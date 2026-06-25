#include "SkillTreeEntryWidget.h"
#include "Skills/SkillDefinition.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Font.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"

namespace EntryStyle
{
    static const FLinearColor BtnNormal (0.10f, 0.16f, 0.28f, 1.0f);
    static const FLinearColor BtnHover  (0.20f, 0.34f, 0.56f, 1.0f);
    static const FLinearColor BtnPress  (0.30f, 0.50f, 0.85f, 1.0f);
    static const FLinearColor Accent    (0.62f, 0.82f, 1.0f, 1.0f);
    static const FLinearColor Text      (0.93f, 0.95f, 0.98f, 1.0f);

    static FSlateFontInfo Font(float Size)
    {
        FSlateFontInfo Info;
        if (UFont* F = LoadObject<UFont>(nullptr, TEXT("/Game/INVENTORY/Other/Fonts/American_Captain_Font")))
        {
            Info.FontObject = F;
        }
        Info.Size = Size;
        return Info;
    }
}

UButton* USkillTreeEntryWidget::MakeSlotButton(const FString& Label)
{
    using namespace EntryStyle;

    UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

    FButtonStyle S = Btn->GetStyle();
    S.Normal  = FSlateRoundedBoxBrush(BtnNormal, 6.f);
    S.Hovered = FSlateRoundedBoxBrush(BtnHover, 6.f);
    S.Pressed = FSlateRoundedBoxBrush(BtnPress, 6.f);
    Btn->SetStyle(S);

    UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    T->SetText(FText::FromString(Label));
    T->SetJustification(ETextJustify::Center);
    T->SetFont(Font(16.f));
    T->SetColorAndOpacity(FSlateColor(Accent));
    Btn->AddChild(T);
    return Btn;
}

TSharedRef<SWidget> USkillTreeEntryWidget::RebuildWidget()
{
    using namespace EntryStyle;

    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
        WidgetTree->RootWidget = Row;

        NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
        NameText->SetFont(Font(18.f));
        NameText->SetColorAndOpacity(FSlateColor(Text));
        if (UHorizontalBoxSlot* NS = Row->AddChildToHorizontalBox(NameText))
        {
            NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            NS->SetVerticalAlignment(VAlign_Center);
            NS->SetPadding(FMargin(6.f, 4.f));
        }

        const TCHAR* Labels[3] = { TEXT("Z"), TEXT("X"), TEXT("C") };
        for (int32 i = 0; i < 3; ++i)
        {
            UButton* Btn = MakeSlotButton(Labels[i]);
            if (i == 0) { Btn->OnClicked.AddDynamic(this, &USkillTreeEntryWidget::OnSlot0); }
            else if (i == 1) { Btn->OnClicked.AddDynamic(this, &USkillTreeEntryWidget::OnSlot1); }
            else { Btn->OnClicked.AddDynamic(this, &USkillTreeEntryWidget::OnSlot2); }

            USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
            Sz->SetWidthOverride(38.f);
            Sz->SetHeightOverride(32.f);
            Sz->AddChild(Btn);
            if (UHorizontalBoxSlot* BS = Row->AddChildToHorizontalBox(Sz))
            {
                BS->SetVerticalAlignment(VAlign_Center);
                BS->SetPadding(FMargin(3.f, 0.f));
            }
        }
    }

    ApplyName();
    return Super::RebuildWidget();
}

void USkillTreeEntryWidget::ApplyName()
{
    if (NameText == nullptr)
    {
        return;
    }
    const FText N = (CachedSkill != nullptr && CachedSkill->SkillName.IsEmpty() == false)
        ? CachedSkill->SkillName
        : FText::FromString(CachedSkill != nullptr ? CachedSkill->GetName() : TEXT("None"));
    NameText->SetText(N);

    // 이름을 등급색으로
    if (CachedSkill != nullptr)
    {
        NameText->SetColorAndOpacity(FSlateColor(SkillRarityColor(CachedSkill->Rarity)));
    }
}

void USkillTreeEntryWidget::InitEntry(int32 InPoolIndex, USkillDefinition* InSkill)
{
    PoolIndex = InPoolIndex;
    CachedSkill = InSkill;
    ApplyName();
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
