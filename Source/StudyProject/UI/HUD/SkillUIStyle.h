#pragma once

#include "CoreMinimal.h"
#include "Components/Border.h"
#include "Components/BackgroundBlur.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Font.h"
#include "Materials/MaterialInterface.h"
#include "Styling/SlateBrush.h"

/**
 * AGIS(AdvancedGridInventorySystem) UI 톤 재현 헬퍼.
 * 패널 = 검정 반투명(0,0,0,0.55) + BackgroundBlur + 회색 외곽선(M_UmgOutline).
 */
namespace SkillUI
{
    // 밝은 레벨에서도 AGIS(어두운 방)처럼 보이도록 패널을 더 불투명하게.
    inline FLinearColor PanelFill()  { return FLinearColor(0.f, 0.f, 0.f, 0.82f); }
    inline FLinearColor SlotFill()   { return FLinearColor(0.f, 0.f, 0.f, 0.88f); }
    inline FLinearColor OutlineCol() { return FLinearColor(0.281f, 0.281f, 0.281f, 0.55f); }
    inline FLinearColor TextMain()   { return FLinearColor(0.90f, 0.91f, 0.93f, 1.f); }
    inline FLinearColor TextDim()    { return FLinearColor(0.62f, 0.66f, 0.72f, 1.f); }

    inline FSlateFontInfo Font(float Size, bool bOutline = false)
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

    inline UMaterialInterface* OutlineMat()
    {
        return LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/INVENTORY/UI/Widgets/Other/WidgetTextures/Materials/M_UmgOutline"));
    }

    // 회색 9-slice 외곽선 보더 브러시 적용
    inline void ApplyOutline(UBorder* B, FLinearColor Tint)
    {
        if (B == nullptr) { return; }
        if (UMaterialInterface* M = OutlineMat())
        {
            FSlateBrush Br;
            Br.SetResourceObject(M);
            Br.DrawAs = ESlateBrushDrawType::Border;
            Br.Margin = FMargin(0.5f);
            Br.TintColor = FSlateColor(Tint);
            B->SetBrush(Br);
        }
        else
        {
            B->SetBrushColor(Tint);
        }
    }

    // 컨텐츠를 AGIS 패널로 감싼다: 외곽선(M_UmgOutline) > BackgroundBlur > 검정반투명 보더 > content.
    // 반환값을 캔버스/슬롯에 배치.
    inline UWidget* MakePanel(UWidgetTree* T, UWidget* Content, FMargin InnerPad, float Blur = 24.f)
    {
        UBorder* Outline = T->ConstructWidget<UBorder>(UBorder::StaticClass());
        ApplyOutline(Outline, OutlineCol());
        Outline->SetPadding(FMargin(3.f));

        UBackgroundBlur* BlurW = T->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass());
        BlurW->SetBlurStrength(Blur);
        Outline->SetContent(BlurW);

        UBorder* Fill = T->ConstructWidget<UBorder>(UBorder::StaticClass());
        Fill->SetBrushColor(PanelFill());
        Fill->SetPadding(InnerPad);
        BlurW->SetContent(Fill);

        Fill->SetContent(Content);
        return Outline;
    }
}
