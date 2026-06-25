#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/BackgroundBlur.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

/**
 * MooresRPGTemplate UI 톤 재현 헬퍼 (통합 탭 메뉴용).
 * 패널 = 검정 반투명 + BackgroundBlur(3) + 코너패턴/가로줄 장식.
 * 버튼 = 둥근 사각 회색(Normal 0.495 / Hover 0.724 / Press 0.384), Roboto Bold 대문자.
 * 텍스처는 Moore에서 경로보존 복사: /Game/MRPGT/Textures/Borders/...
 */
namespace MenuUI
{
    inline FLinearColor PanelFill() { return FLinearColor(0.f, 0.f, 0.f, 0.86f); }
    inline FLinearColor TabTextNormal()   { return FLinearColor(0.82f, 0.82f, 0.84f, 1.f); }
    inline FLinearColor TabTextActive()   { return FLinearColor(0.99f, 0.98f, 0.94f, 1.f); }

    inline UTexture2D* Tex(const TCHAR* Path)
    {
        return LoadObject<UTexture2D>(nullptr, Path);
    }
    inline UTexture2D* CornerTex()     { return Tex(TEXT("/Game/MRPGT/Textures/Borders/T_Corner_Pattern05.T_Corner_Pattern05")); }
    inline UTexture2D* HorizLineTex()  { return Tex(TEXT("/Game/MRPGT/Textures/Borders/T_Horiz_Line01.T_Horiz_Line01")); }
    inline UTexture2D* DoubleLineTex() { return Tex(TEXT("/Game/MRPGT/Textures/Borders/T_Double_Horiz_Line01.T_Double_Horiz_Line01")); }

    inline UFont* RobotoFont()
    {
        // 엔진 기본 Roboto (Moore 인벤토리와 동일 폰트)
        return LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    }

    inline FSlateFontInfo Font(float Size, const FName& Typeface = FName("Bold"))
    {
        FSlateFontInfo Info;
        if (UFont* F = RobotoFont())
        {
            Info.FontObject = F;
            Info.TypefaceFontName = Typeface;
        }
        Info.Size = Size;
        return Info;
    }

    // Moore 탭/액션 버튼 스타일: 둥근 사각 + 회색 단색.
    inline FSlateBrush RoundedFill(FLinearColor C, float Radius = 6.f)
    {
        FSlateBrush B;
        B.DrawAs = ESlateBrushDrawType::RoundedBox;
        B.TintColor = FSlateColor(C);
        B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return B;
    }

    inline FButtonStyle ButtonStyle()
    {
        FButtonStyle S;
        S.SetNormal(RoundedFill(FLinearColor(0.10f, 0.10f, 0.12f, 0.85f), 3.f));
        S.SetHovered(RoundedFill(FLinearColor(0.20f, 0.16f, 0.10f, 0.92f), 3.f));
        S.SetPressed(RoundedFill(FLinearColor(0.06f, 0.06f, 0.07f, 0.95f), 3.f));
        S.SetNormalPadding(FMargin(16.f, 8.f));
        S.SetPressedPadding(FMargin(16.f, 9.f, 16.f, 7.f));
        return S;
    }

    // 선택된 탭 버튼은 밝게(강조)
    inline FButtonStyle TabButtonStyle(bool bActive)
    {
        FButtonStyle S;
        // 회색 박스 제거 — 탭은 투명 배경 + 텍스트색/밑줄로 선택 표시(Moore 방식).
        S.SetNormal(RoundedFill(FLinearColor(0.f, 0.f, 0.f, 0.f), 3.f));
        S.SetHovered(RoundedFill(FLinearColor(1.f, 1.f, 1.f, bActive ? 0.10f : 0.06f), 3.f));
        S.SetPressed(RoundedFill(FLinearColor(1.f, 1.f, 1.f, 0.04f), 3.f));
        S.SetNormalPadding(FMargin(28.f, 10.f, 28.f, 7.f));
        S.SetPressedPadding(FMargin(28.f, 11.f, 28.f, 6.f));
        return S;
    }

    // 필터/정렬/로드아웃 같은 작은 칩 버튼: 투명 배경 + 활성 시 따뜻한 옅은 하이라이트.
    inline FButtonStyle ChipButtonStyle(bool bActive)
    {
        FButtonStyle S;
        const FLinearColor N = bActive ? FLinearColor(0.95f, 0.78f, 0.30f, 0.22f) : FLinearColor(1.f, 1.f, 1.f, 0.f);
        const FLinearColor H = bActive ? FLinearColor(0.97f, 0.80f, 0.34f, 0.32f) : FLinearColor(1.f, 1.f, 1.f, 0.08f);
        const FLinearColor P = bActive ? FLinearColor(0.90f, 0.74f, 0.28f, 0.18f) : FLinearColor(1.f, 1.f, 1.f, 0.05f);
        S.SetNormal(RoundedFill(N, 3.f));
        S.SetHovered(RoundedFill(H, 3.f));
        S.SetPressed(RoundedFill(P, 3.f));
        S.SetNormalPadding(FMargin(12.f, 5.f));
        S.SetPressedPadding(FMargin(12.f, 6.f, 12.f, 4.f));
        return S;
    }

    // 일반 액션 버튼(강화 실행 등): 어두운 반투명 + 호버 시 따뜻하게.
    inline FButtonStyle ActionButtonStyle()
    {
        FButtonStyle S;
        S.SetNormal(RoundedFill(FLinearColor(0.12f, 0.12f, 0.14f, 0.90f), 4.f));
        S.SetHovered(RoundedFill(FLinearColor(0.24f, 0.19f, 0.10f, 0.95f), 4.f));
        S.SetPressed(RoundedFill(FLinearColor(0.07f, 0.07f, 0.08f, 0.96f), 4.f));
        S.SetNormalPadding(FMargin(18.f, 9.f));
        S.SetPressedPadding(FMargin(18.f, 10.f, 18.f, 8.f));
        return S;
    }

    // 가로줄 장식 이미지 브러시 (텍스처 가로 타일)
    inline FSlateBrush LineBrush(UTexture2D* T, float Height = 8.f)
    {
        FSlateBrush B;
        if (T)
        {
            B.SetResourceObject(T);
            B.DrawAs = ESlateBrushDrawType::Image;
            B.ImageSize = FVector2D(64.f, Height);
            B.Tiling = ESlateBrushTileType::Horizontal;
        }
        return B;
    }
}
