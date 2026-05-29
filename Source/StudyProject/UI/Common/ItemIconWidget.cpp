#include "ItemIconWidget.h"
#include "Components/Image.h"
#include "Components/Border.h"

void UItemIconWidget::SetItemData(const FItemData& Data)
{
    if (Data.Icon.IsValid())
    {
        SetIconTexture(Data.Icon.Get());
    }
    else
    {
        SetIconTexture(nullptr);
    }
    SetRarity(Data.Rarity);
}

void UItemIconWidget::SetIconTexture(UTexture2D* Texture)
{
    if (IconImage == nullptr)
    {
        return;
    }

    if (Texture)
    {
        IconImage->SetBrushFromTexture(Texture);
        IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
    else
    {
        IconImage->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UItemIconWidget::SetRarity(EItemRarity Rarity)
{
    if (RarityBorder == nullptr)
    {
        return;
    }
    RarityBorder->SetBrushColor(GetRarityColor(Rarity));
}

void UItemIconWidget::Clear()
{
    if (IconImage)
    {
        IconImage->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (RarityBorder)
    {
        RarityBorder->SetBrushColor(GetRarityColor(EItemRarity::Common));
    }
}

FLinearColor UItemIconWidget::GetRarityColor(EItemRarity Rarity)
{
    switch (Rarity)
    {
        case EItemRarity::Common:    return FLinearColor(0.25f, 0.25f, 0.25f, 1.f);
        case EItemRarity::Uncommon:  return FLinearColor(0.05f, 0.55f, 0.05f, 1.f);
        case EItemRarity::Rare:      return FLinearColor(0.10f, 0.35f, 0.90f, 1.f);
        case EItemRarity::Epic:      return FLinearColor(0.55f, 0.05f, 0.90f, 1.f);
        case EItemRarity::Legendary: return FLinearColor(0.95f, 0.50f, 0.05f, 1.f);
        default:                     return FLinearColor(0.25f, 0.25f, 0.25f, 1.f);
    }
}
