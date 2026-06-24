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

namespace
{
    const TCHAR* SlotHotkey(int32 Index)
    {
        switch (Index)
        {
        case 0: return TEXT("Z");
        case 1: return TEXT("X");
        case 2: return TEXT("C");
        default: return TEXT("");
        }
    }
}

TSharedRef<SWidget> USkillHUDWidget::RebuildWidget()
{
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
            // 하단 중앙
            CS->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
            CS->SetAlignment(FVector2D(0.5f, 1.f));
            CS->SetPosition(FVector2D(0.f, -40.f));
            CS->SetAutoSize(true);
        }
    }
    return Super::RebuildWidget();
}

void USkillHUDWidget::BuildSlot(int32 Index, UHorizontalBox* Row)
{
    USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Sz%d"), Index));
    Sz->SetWidthOverride(72.f);
    Sz->SetHeightOverride(72.f);

    UOverlay* Ov = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *FString::Printf(TEXT("Ov%d"), Index));
    Sz->AddChild(Ov);

    // 슬롯 배경(빈 UImage는 단색이 안 그려져 Border 사용)
    UBorder* Bg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("Bg%d"), Index));
    Bg->SetBrushColor(EmptySlotColor);
    Ov->AddChildToOverlay(Bg);
    SlotBgBorders[Index] = Bg;

    // 아이콘(텍스처 있을 때만 표시)
    UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("Icon%d"), Index));
    Icon->SetVisibility(ESlateVisibility::Collapsed);
    Ov->AddChildToOverlay(Icon);
    IconImages[Index] = Icon;

    // 쿨다운 어둡게 덮개(Border)
    UBorder* Cd = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("Cd%d"), Index));
    Cd->SetBrushColor(CooldownTint);
    Cd->SetVisibility(ESlateVisibility::Collapsed);
    Ov->AddChildToOverlay(Cd);
    CooldownOverlays[Index] = Cd;

    // 쿨다운 카운트다운 텍스트(중앙)
    UTextBlock* CdText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("CdText%d"), Index));
    CdText->SetJustification(ETextJustify::Center);
    FSlateFontInfo CF = CdText->GetFont();
    CF.Size = 24.f;
    CdText->SetFont(CF);
    CdText->SetText(FText::GetEmpty());
    if (UOverlaySlot* OS = Ov->AddChildToOverlay(CdText))
    {
        OS->SetHorizontalAlignment(HAlign_Center);
        OS->SetVerticalAlignment(VAlign_Center);
    }
    CooldownTexts[Index] = CdText;

    // 단축키 라벨(좌상단)
    UTextBlock* Key = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Key%d"), Index));
    Key->SetText(FText::FromString(SlotHotkey(Index)));
    FSlateFontInfo KF = Key->GetFont();
    KF.Size = 14.f;
    Key->SetFont(KF);
    Key->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.5f)));
    if (UOverlaySlot* KS = Ov->AddChildToOverlay(Key))
    {
        KS->SetHorizontalAlignment(HAlign_Left);
        KS->SetVerticalAlignment(VAlign_Top);
        KS->SetPadding(FMargin(4.f, 2.f, 0.f, 0.f));
    }

    if (UHorizontalBoxSlot* HS = Row->AddChildToHorizontalBox(Sz))
    {
        HS->SetPadding(FMargin(6.f, 0.f));
    }
}

void USkillHUDWidget::InitHUD(USkillManagerComponent* InComp)
{
    SkillComp = InComp;
}

void USkillHUDWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
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

        // 슬롯 배경색(스킬 있음/없음) + 아이콘
        SlotBgBorders[i]->SetBrushColor(Skill != nullptr ? FilledSlotColor : EmptySlotColor);
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

        // 쿨다운 표시
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
