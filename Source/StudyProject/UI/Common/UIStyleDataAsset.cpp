#include "UIStyleDataAsset.h"

FLinearColor UUIStyleDataAsset::GetRarityColor(EItemRarity Rarity) const
{
    const FLinearColor* Found = RarityColors.Find(Rarity);
    return Found ? *Found : FLinearColor::White;
}
