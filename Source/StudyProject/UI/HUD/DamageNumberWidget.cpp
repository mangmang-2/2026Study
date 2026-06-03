#include "DamageNumberWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/PlayerController.h"

TSharedRef<SWidget> UDamageNumberWidget::RebuildWidget()
{
    // 코드로 위젯 트리 구성(루트 캔버스 + 텍스트)
    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
        WidgetTree->RootWidget = Canvas;

        DamageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageText"));
        if (UCanvasPanelSlot* TextSlot = Canvas->AddChildToCanvas(DamageText))
        {
            TextSlot->SetAutoSize(true);
        }
    }
    return Super::RebuildWidget();
}

void UDamageNumberWidget::Init(int32 Damage, EDamageType Type, FVector WorldPos)
{
    WorldLoc = WorldPos;
    Elapsed = 0.f;
    bInitialized = true;

    if (DamageText == nullptr)
    {
        return;
    }

    DamageText->SetText(FText::AsNumber(Damage));

    // 타입별 색/크기
    FLinearColor Color = FLinearColor::White;
    float FontSize = 26.f;
    if (Type == EDamageType::Critical)
    {
        Color = FLinearColor(1.f, 0.85f, 0.1f);   // 노랑
        FontSize = 38.f;
    }
    else if (Type == EDamageType::Heal)
    {
        Color = FLinearColor(0.3f, 1.f, 0.3f);     // 초록
    }
    DamageText->SetColorAndOpacity(FSlateColor(Color));

    FSlateFontInfo Font = DamageText->GetFont();
    Font.Size = FontSize;
    DamageText->SetFont(Font);
}

void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
    Super::NativeTick(MyGeometry, DeltaTime);

    if (bInitialized == false)
    {
        return;
    }

    Elapsed += DeltaTime;

    APlayerController* PC = GetOwningPlayer();
    if (PC == nullptr)
    {
        RemoveFromParent();
        return;
    }

    // 월드 → 스크린 투영 + 시간에 따라 위로 상승
    FVector2D Screen;
    if (PC->ProjectWorldLocationToScreen(WorldLoc, Screen, false))
    {
        const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
        FVector2D Pos = (ViewportScale > 0.f) ? (Screen / ViewportScale) : Screen;
        Pos.Y -= Elapsed * RiseSpeed;
        SetPositionInViewport(Pos, false);
    }

    // 페이드아웃 + 수명 종료
    const float Alpha = FMath::Clamp(1.f - (Elapsed / Lifetime), 0.f, 1.f);
    SetRenderOpacity(Alpha);

    if (Elapsed >= Lifetime)
    {
        RemoveFromParent();
    }
}
