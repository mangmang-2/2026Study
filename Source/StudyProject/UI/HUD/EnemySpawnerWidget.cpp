#include "EnemySpawnerWidget.h"
#include "Character/PlayerCharacter.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> UEnemySpawnerWidget::RebuildWidget()
{
    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
        WidgetTree->RootWidget = Canvas;

        UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));

        auto MakeButton = [&](const TCHAR* BtnName, const TCHAR* TextName, const FString& Label) -> UButton*
        {
            UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), BtnName);
            UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
            T->SetText(FText::FromString(Label));
            T->SetJustification(ETextJustify::Center);
            B->AddChild(T);
            if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(B))
            {
                S->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            }
            return B;
        };

        SpawnEnemyBtn = MakeButton(TEXT("SpawnEnemyBtn"), TEXT("T1"), TEXT("적 소환"));
        SpawnBossBtn  = MakeButton(TEXT("SpawnBossBtn"),  TEXT("T2"), TEXT("보스 소환"));
        ClearBtn      = MakeButton(TEXT("ClearBtn"),      TEXT("T3"), TEXT("전체 제거"));

        SpawnEnemyBtn->OnClicked.AddDynamic(this, &UEnemySpawnerWidget::OnSpawnEnemy);
        SpawnBossBtn->OnClicked.AddDynamic(this, &UEnemySpawnerWidget::OnSpawnBoss);
        ClearBtn->OnClicked.AddDynamic(this, &UEnemySpawnerWidget::OnClear);

        if (UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Box))
        {
            // 우하단 고정(X 닫기 버튼 안 가리게)
            CS->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
            CS->SetAlignment(FVector2D(1.f, 1.f));
            CS->SetPosition(FVector2D(-20.f, -20.f));
            CS->SetAutoSize(true);
        }
    }
    return Super::RebuildWidget();
}

APlayerCharacter* UEnemySpawnerWidget::GetPlayerChar() const
{
    return Cast<APlayerCharacter>(GetOwningPlayerPawn());
}

void UEnemySpawnerWidget::OnSpawnEnemy()
{
    if (APlayerCharacter* P = GetPlayerChar()) { P->SpawnTestEnemy(); }
}

void UEnemySpawnerWidget::OnSpawnBoss()
{
    if (APlayerCharacter* P = GetPlayerChar()) { P->SpawnTestBoss(); }
}

void UEnemySpawnerWidget::OnClear()
{
    if (APlayerCharacter* P = GetPlayerChar()) { P->ClearAllEnemies(); }
}
