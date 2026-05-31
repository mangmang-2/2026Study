#include "ItemIconWidget.h"
#include "Components/Image.h"
#include "Components/Border.h"

void UItemIconWidget::SetItemData(const FItemData& Data)
{
    // 소프트 포인터는 아직 미로드 상태일 수 있으므로 동기 로드 (IsValid는 로드된 것만 true)
    UTexture2D* Tex = Data.Icon.IsNull() ? nullptr : Data.Icon.LoadSynchronous();
    SetIconTexture(Tex);
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
        // WBP 기본 ColorAndOpacity 알파가 0이면 투명하게 렌더되므로 흰색 불투명으로 강제
        IconImage->SetColorAndOpacity(FLinearColor::White);
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
