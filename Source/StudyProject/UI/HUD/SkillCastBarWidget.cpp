#include "SkillCastBarWidget.h"
#include "Skills/SkillManagerComponent.h"
#include "Skills/SkillDefinition.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> USkillCastBarWidget::RebuildWidget()
{
    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
        WidgetTree->RootWidget = Canvas;

        RootBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootBox"));
        RootBox->SetWidthOverride(420.f);
        RootBox->SetVisibility(ESlateVisibility::Collapsed);

        UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
        RootBox->AddChild(Box);

        Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
        Label->SetJustification(ETextJustify::Center);
        FSlateFontInfo F = Label->GetFont();
        F.Size = 16.f;
        Label->SetFont(F);
        Box->AddChildToVerticalBox(Label);

        Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Bar"));
        Bar->SetFillColorAndOpacity(FLinearColor(0.3f, 0.6f, 1.f));
        if (UVerticalBoxSlot* BS = Box->AddChildToVerticalBox(Bar))
        {
            BS->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
        }

        if (UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(RootBox))
        {
            // 화면 중앙보다 약간 아래
            CS->SetAnchors(FAnchors(0.5f, 0.65f, 0.5f, 0.65f));
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
