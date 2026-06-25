#include "SkillCastBarWorldWidget.h"
#include "Skills/SkillManagerComponent.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"

TSharedRef<SWidget> USkillCastBarWorldWidget::RebuildWidget()
{
    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
        WidgetTree->RootWidget = Canvas;

        Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Bar"));
        const FLinearColor Fill(0.32f, 0.64f, 1.0f, 1.0f);
        Bar->SetFillColorAndOpacity(Fill);
        FProgressBarStyle PS = Bar->GetWidgetStyle();
        PS.BackgroundImage = FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.03f, 0.05f, 0.85f), 4.f, FLinearColor(0.3f, 0.5f, 0.9f, 1.f), 1.f);
        PS.FillImage = FSlateRoundedBoxBrush(Fill, 4.f);
        Bar->SetWidgetStyle(PS);
        Bar->SetVisibility(ESlateVisibility::Collapsed);

        if (UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Bar))
        {
            CS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            CS->SetOffsets(FMargin(0.f));
        }
    }
    return Super::RebuildWidget();
}

void USkillCastBarWorldWidget::InitWorldCastBar(USkillManagerComponent* InComp)
{
    SkillComp = InComp;
}

void USkillCastBarWorldWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
    Super::NativeTick(MyGeometry, DeltaTime);

    if (SkillComp.IsValid() == false || Bar == nullptr)
    {
        return;
    }

    if (SkillComp->IsWorldCasting())
    {
        Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
        Bar->SetPercent(SkillComp->GetWorldCastProgress());
    }
    else
    {
        Bar->SetVisibility(ESlateVisibility::Collapsed);
    }
}
