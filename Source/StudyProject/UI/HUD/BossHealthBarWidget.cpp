#include "BossHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"

TSharedRef<SWidget> UBossHealthBarWidget::RebuildWidget()
{
    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
        WidgetTree->RootWidget = Canvas;

        UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));

        NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
        NameText->SetJustification(ETextJustify::Center);
        FSlateFontInfo F = NameText->GetFont(); F.Size = 22.f; NameText->SetFont(F);
        Box->AddChildToVerticalBox(NameText);

        HPBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HPBar"));
        HPBar->SetFillColorAndOpacity(FLinearColor(0.8f, 0.05f, 0.05f));
        if (UVerticalBoxSlot* BarSlot = Box->AddChildToVerticalBox(HPBar))
        {
            BarSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
            BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }

        if (UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Box))
        {
            // 상단 중앙 가로로 넓게
            CS->SetAnchors(FAnchors(0.5f, 0.f, 0.5f, 0.f));
            CS->SetAlignment(FVector2D(0.5f, 0.f));
            CS->SetPosition(FVector2D(0.f, 40.f));
            CS->SetSize(FVector2D(720.f, 40.f));
        }
    }
    return Super::RebuildWidget();
}

void UBossHealthBarWidget::SetBoss(AActor* InBoss, const FText& InName)
{
    Boss = InBoss;
    if (NameText)
    {
        NameText->SetText(InName);
    }
}

void UBossHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
    Super::NativeTick(MyGeometry, DeltaTime);

    if (Boss.IsValid() == false)
    {
        RemoveFromParent();
        return;
    }

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Boss.Get());
    if (ASC == nullptr || HPBar == nullptr)
    {
        return;
    }

    const float HP = ASC->GetNumericAttribute(UCombatAttributeSet::GetHPAttribute());
    const float MaxHP = ASC->GetNumericAttribute(UCombatAttributeSet::GetMaxHPAttribute());
    HPBar->SetPercent((MaxHP > 0.f) ? FMath::Clamp(HP / MaxHP, 0.f, 1.f) : 0.f);
}
