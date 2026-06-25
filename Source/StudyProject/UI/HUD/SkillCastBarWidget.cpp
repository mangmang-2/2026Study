#include "SkillCastBarWidget.h"
#include "Skills/SkillManagerComponent.h"
#include "Skills/SkillDefinition.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Font.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "SkillUIStyle.h"

namespace CastBarStyle
{
    static const FLinearColor Panel  (0.02f, 0.03f, 0.05f, 0.92f);
    static const FLinearColor Outline(0.34f, 0.55f, 0.95f, 1.0f);
    static const FLinearColor Fill   (0.30f, 0.62f, 1.0f, 1.0f);
    static const FLinearColor Track  (0.10f, 0.12f, 0.18f, 1.0f);
    static const FLinearColor Text   (0.93f, 0.95f, 0.98f, 1.0f);

    static FSlateFontInfo Font(float Size)
    {
        FSlateFontInfo Info;
        if (UFont* F = LoadObject<UFont>(nullptr, TEXT("/Game/INVENTORY/Other/Fonts/American_Captain_Font")))
        {
            Info.FontObject = F;
        }
        Info.Size = Size;
        Info.OutlineSettings.OutlineSize = 1;
        Info.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
        return Info;
    }
}

TSharedRef<SWidget> USkillCastBarWidget::RebuildWidget()
{
    using namespace CastBarStyle;

    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
        WidgetTree->RootWidget = Canvas;

        RootBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootBox"));
        RootBox->SetWidthOverride(440.f);
        RootBox->SetVisibility(ESlateVisibility::Collapsed);

        UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
        // AGIS 패널(블러+검정반투명+회색외곽선)로 감쌈
        UWidget* PanelB = SkillUI::MakePanel(WidgetTree, Box, FMargin(14.f, 8.f));
        RootBox->AddChild(PanelB);

        Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
        Label->SetJustification(ETextJustify::Center);
        Label->SetFont(Font(18.f));
        Label->SetColorAndOpacity(FSlateColor(Text));
        Box->AddChildToVerticalBox(Label);

        // 게이지 트랙(둥근) + 채움 바
        UBorder* TrackB = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Track"));
        TrackB->SetBrush(FSlateRoundedBoxBrush(Track, 5.f));
        TrackB->SetPadding(FMargin(1.f));
        if (UVerticalBoxSlot* TS = Box->AddChildToVerticalBox(TrackB))
        {
            TS->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));
        }

        Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Bar"));
        Bar->SetFillColorAndOpacity(Fill);
        FProgressBarStyle PS = Bar->GetWidgetStyle();
        PS.BackgroundImage = FSlateRoundedBoxBrush(FLinearColor::Transparent, 5.f);
        PS.FillImage = FSlateRoundedBoxBrush(Fill, 5.f);
        Bar->SetWidgetStyle(PS);
        TrackB->AddChild(Bar);

        if (UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(RootBox))
        {
            CS->SetAnchors(FAnchors(0.5f, 0.68f, 0.5f, 0.68f));
            CS->SetAlignment(FVector2D(0.5f, 0.5f));
            CS->SetPosition(FVector2D(0.f, 0.f));
            CS->SetAutoSize(true);
        }
    }
    return Super::RebuildWidget();
}

void USkillCastBarWidget::InitCastBar(USkillManagerComponent* InComp)
{
    SkillComp = InComp;
}

void USkillCastBarWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
    Super::NativeTick(MyGeometry, DeltaTime);

    if (SkillComp.IsValid() == false || RootBox == nullptr)
    {
        return;
    }

    if (SkillComp->IsCasting())
    {
        RootBox->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (Bar != nullptr)
        {
            Bar->SetPercent(SkillComp->GetCastProgress());
        }
        if (Label != nullptr)
        {
            USkillDefinition* Skill = SkillComp->GetCastingSkill();
            Label->SetText(Skill != nullptr ? Skill->SkillName : FText::GetEmpty());
        }
    }
    else
    {
        RootBox->SetVisibility(ESlateVisibility::Collapsed);
    }
}
