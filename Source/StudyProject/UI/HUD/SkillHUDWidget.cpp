#include "SkillHUDWidget.h"
#include "Skills/SkillManagerComponent.h"
#include "Skills/SkillDefinition.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "SkillUIStyle.h"

namespace SkillHUDStyle
{
    static const FLinearColor SlotEmpty  (0.05f, 0.06f, 0.09f, 0.92f);
    static const FLinearColor SlotFilled (0.09f, 0.12f, 0.19f, 0.96f);
    static const FLinearColor OutlineDim (0.18f, 0.21f, 0.30f, 1.0f);
    static const FLinearColor OutlineLit (0.34f, 0.55f, 0.95f, 1.0f);
    static const FLinearColor Accent     (0.55f, 0.78f, 1.0f, 1.0f);
    static const FLinearColor CooldownDim(0.0f, 0.0f, 0.0f, 0.72f);
    static const FLinearColor TextMain   (0.93f, 0.95f, 0.98f, 1.0f);

    static const TCHAR* SlotKey(int32 Index)
    {
        switch (Index)
        {
        case 0: return TEXT("Z");
        case 1: return TEXT("X");
        case 2: return TEXT("C");
        default: return TEXT("");
        }
    }

    // 등급색을 UI 톤에 맞게 채도 낮춘 버전(테두리용)
    static FLinearColor Tone(const FLinearColor& C)
    {
        const float G = (C.R + C.G + C.B) / 3.f;
        const float D = 0.42f; // 회색 쪽으로 섞어 채도↓
        return FLinearColor(FMath::Lerp(C.R, G, D), FMath::Lerp(C.G, G, D), FMath::Lerp(C.B, G, D), 1.f);
    }

    static FSlateFontInfo Font(float Size, bool bOutline)
    {
        FSlateFontInfo Info;
        if (UFont* F = LoadObject<UFont>(nullptr, TEXT("/Game/INVENTORY/Other/Fonts/American_Captain_Font")))
        {
            Info.FontObject = F;
        }
        Info.Size = Size;
        if (bOutline)
        {
            Info.OutlineSettings.OutlineSize = 1;
            Info.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
        }
        return Info;
    }
}

TSharedRef<SWidget> USkillHUDWidget::RebuildWidget()
{
    using namespace SkillHUDStyle;

    if (WidgetTree && WidgetTree->RootWidget == nullptr)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
        WidgetTree->RootWidget = Canvas;

        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));

        SlotBgBorders.SetNum(NumSlots);
        IconImages.SetNum(NumSlots);
        CooldownOverlays.SetNum(NumSlots);
        CooldownTexts.SetNum(NumSlots);

        for (int32 i = 0; i < NumSlots; ++i)
        {
            BuildSlot(i, Row);
        }

        if (UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Row))
        {
            CS->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));   // 좌하단 기준
            CS->SetAlignment(FVector2D(0.f, 1.f));          // 위젯 좌하단을 앵커에 맞춤
            CS->SetPosition(FVector2D(28.f, -96.f));        // 체력바(X28,폭360)와 좌우 끝 일치, 그 위에
            CS->SetAutoSize(true);
        }
    }
    return Super::RebuildWidget();
}

void USkillHUDWidget::BuildSlot(int32 Index, UHorizontalBox* Row)
{
    using namespace SkillHUDStyle;

    // 셀 3개(120×3=360)가 체력바 폭과 일치하도록 함(체력바도 X28,폭360으로 맞춤)
    USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Sz%d"), Index));
    Sz->SetWidthOverride(120.f);
    Sz->SetHeightOverride(120.f);

    UOverlay* Ov = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *FString::Printf(TEXT("Ov%d"), Index));
    Sz->AddChild(Ov);

    // 슬롯 채움 — 검정 반투명(AGIS 톤)
    UBorder* Fill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("Fill%d"), Index));
    Fill->SetBrushColor(SkillUI::SlotFill());
    Fill->SetPadding(FMargin(0.f));
    if (UOverlaySlot* FS = Ov->AddChildToOverlay(Fill))
    {
        FS->SetHorizontalAlignment(HAlign_Fill);
        FS->SetVerticalAlignment(VAlign_Fill);
    }

    // 아이콘
    UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("Icon%d"), Index));
    Icon->SetVisibility(ESlateVisibility::Collapsed);
    if (UOverlaySlot* IS = Ov->AddChildToOverlay(Icon))
    {
        IS->SetHorizontalAlignment(HAlign_Fill);
        IS->SetVerticalAlignment(VAlign_Fill);
        IS->SetPadding(FMargin(2.f));
    }
    IconImages[Index] = Icon;

    // 쿨다운 어둡게 덮개 — 셀을 꽉 채우는 사각(버튼 안에 정확히 들어오게)
    UBorder* Cd = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("Cd%d"), Index));
    Cd->SetBrushColor(CooldownDim);
    Cd->SetPadding(FMargin(0.f));
    Cd->SetVisibility(ESlateVisibility::Collapsed);
    if (UOverlaySlot* CS = Ov->AddChildToOverlay(Cd))
    {
        CS->SetHorizontalAlignment(HAlign_Fill);
        CS->SetVerticalAlignment(VAlign_Fill);
    }
    CooldownOverlays[Index] = Cd;

    // 등급색 외곽선 — 얇은(2px) 둥근 테두리. Tick에서 톤다운한 등급색으로 갱신. 아이콘/덮개 위 레이어.
    UBorder* Outline = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("Outline%d"), Index));
    Outline->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.f), 3.f, SkillUI::OutlineCol(), 2.f));
    Outline->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UOverlaySlot* OLS = Ov->AddChildToOverlay(Outline))
    {
        OLS->SetHorizontalAlignment(HAlign_Fill);
        OLS->SetVerticalAlignment(VAlign_Fill);
    }
    SlotBgBorders[Index] = Outline;

    // 쿨다운 카운트다운(중앙)
    UTextBlock* CdText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("CdText%d"), Index));
    CdText->SetJustification(ETextJustify::Center);
    CdText->SetFont(Font(34.f, true));
    CdText->SetColorAndOpacity(FSlateColor(TextMain));
    CdText->SetText(FText::GetEmpty());
    if (UOverlaySlot* OS = Ov->AddChildToOverlay(CdText))
    {
        OS->SetHorizontalAlignment(HAlign_Center);
        OS->SetVerticalAlignment(VAlign_Center);
    }
    CooldownTexts[Index] = CdText;

    // 단축키 칩(좌하단)
    UBorder* KeyChip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("KeyChip%d"), Index));
    KeyChip->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.03f, 0.05f, 0.9f), 6.f, OutlineLit, 1.f));
    KeyChip->SetPadding(FMargin(6.f, 1.f, 6.f, 1.f));
    UTextBlock* Key = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Key%d"), Index));
    Key->SetText(FText::FromString(SlotKey(Index)));
    Key->SetFont(Font(18.f, false));
    Key->SetColorAndOpacity(FSlateColor(Accent));
    KeyChip->AddChild(Key);
    if (UOverlaySlot* KS = Ov->AddChildToOverlay(KeyChip))
    {
        KS->SetHorizontalAlignment(HAlign_Left);
        KS->SetVerticalAlignment(VAlign_Bottom);
        KS->SetPadding(FMargin(4.f, 0.f, 0.f, 4.f));
    }

    if (UHorizontalBoxSlot* HS = Row->AddChildToHorizontalBox(Sz))
    {
        HS->SetPadding(FMargin(0.f)); // 슬롯끼리 붙여 그리드처럼 연속
    }
}

void USkillHUDWidget::InitHUD(USkillManagerComponent* InComp)
{
    SkillComp = InComp;
}

void USkillHUDWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
    using namespace SkillHUDStyle;

    Super::NativeTick(MyGeometry, DeltaTime);

    if (SkillComp.IsValid() == false)
    {
        return;
    }

    for (int32 i = 0; i < NumSlots; ++i)
    {
        if (SlotBgBorders.IsValidIndex(i) == false || SlotBgBorders[i] == nullptr)
        {
            continue;
        }

        USkillDefinition* Skill = SkillComp->GetSlotSkill(i);

        // 얇은 등급색 테두리(톤다운). 비면 회색.
        const FLinearColor BorderCol = (Skill != nullptr) ? Tone(SkillRarityColor(Skill->Rarity)) : SkillUI::OutlineCol();
        SlotBgBorders[i]->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.f), 3.f, BorderCol, 2.f));

        if (IconImages[i] != nullptr)
        {
            if (Skill != nullptr && Skill->Icon != nullptr)
            {
                IconImages[i]->SetBrushFromTexture(Skill->Icon);
                IconImages[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
            else
            {
                IconImages[i]->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        const float Remaining = SkillComp->GetCooldownRemaining(i);
        if (Remaining > 0.f && Skill != nullptr)
        {
            CooldownOverlays[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
            const FString Txt = (Remaining >= 10.f)
                ? FString::Printf(TEXT("%d"), FMath::CeilToInt(Remaining))
                : FString::Printf(TEXT("%.1f"), Remaining);
            CooldownTexts[i]->SetText(FText::FromString(Txt));
        }
        else
        {
            CooldownOverlays[i]->SetVisibility(ESlateVisibility::Collapsed);
            CooldownTexts[i]->SetText(FText::GetEmpty());
        }
    }
}
