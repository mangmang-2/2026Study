#include "TooltipWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetLayoutLibrary.h"

namespace
{
    FLinearColor RarityColor(EItemRarity R)
    {
        switch (R)
        {
        case EItemRarity::Uncommon:  return FLinearColor(0.30f, 0.85f, 0.30f); // 초록
        case EItemRarity::Rare:      return FLinearColor(0.25f, 0.55f, 1.00f); // 파랑
        case EItemRarity::Epic:      return FLinearColor(0.70f, 0.35f, 0.95f); // 보라
        case EItemRarity::Legendary: return FLinearColor(1.00f, 0.65f, 0.15f); // 주황
        default:                     return FLinearColor::White;               // 일반
        }
    }

    const TCHAR* RarityName(EItemRarity R)
    {
        switch (R)
        {
        case EItemRarity::Uncommon:  return TEXT("고급");
        case EItemRarity::Rare:      return TEXT("희귀");
        case EItemRarity::Epic:      return TEXT("영웅");
        case EItemRarity::Legendary: return TEXT("전설");
        default:                     return TEXT("일반");
        }
    }
}

void UTooltipWidget::SetItemData(const FItemData& Data)
{
    if (ItemNameText)
    {
        ItemNameText->SetText(Data.ItemName.IsEmpty() ? FText::FromString(TEXT("(이름 없음)")) : Data.ItemName);
        ItemNameText->SetColorAndOpacity(FSlateColor(RarityColor(Data.Rarity)));
    }

    if (StatsText)
    {
        TArray<FString> Lines;
        Lines.Add(FString::Printf(TEXT("[%s]"), RarityName(Data.Rarity)));
        if (Data.EquipSlot != EEquipSlot::None)
        {
            if (Data.BaseATK != 0) Lines.Add(FString::Printf(TEXT("공격력 +%d"), Data.BaseATK));
            if (Data.BaseDEF != 0) Lines.Add(FString::Printf(TEXT("방어력 +%d"), Data.BaseDEF));
            if (Data.BaseHP  != 0) Lines.Add(FString::Printf(TEXT("체력 +%d"),   Data.BaseHP));
        }
        if (Data.HealAmount > 0) Lines.Add(FString::Printf(TEXT("회복 +%d"), Data.HealAmount));
        if (Data.SellPrice  > 0) Lines.Add(FString::Printf(TEXT("판매가 %d G"), Data.SellPrice));

        StatsText->SetText(FText::FromString(FString::Join(Lines, TEXT("\n"))));
    }

    if (DescriptionText)
    {
        DescriptionText->SetText(Data.Description);
        DescriptionText->SetVisibility(Data.Description.IsEmpty()
            ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    OnItemDataSet(Data);
}

void UTooltipWidget::SetPosition(FVector2D ScreenPos)
{
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
    if (CanvasSlot) CanvasSlot->SetPosition(ScreenPos);
}
