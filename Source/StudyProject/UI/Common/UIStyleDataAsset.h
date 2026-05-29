#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/ItemData.h"
#include "Fonts/SlateFontInfo.h"
#include "UIStyleDataAsset.generated.h"

UCLASS(BlueprintType)
class STUDYPROJECT_API UUIStyleDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
    FLinearColor BackgroundColor = FLinearColor(0.02f, 0.02f, 0.02f, 0.95f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
    FLinearColor PanelColor = FLinearColor(0.08f, 0.08f, 0.1f, 0.9f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
    FLinearColor BorderColor = FLinearColor(0.3f, 0.3f, 0.35f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
    FLinearColor AccentColor = FLinearColor(0.2f, 0.5f, 1.f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
    FLinearColor TextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
    float PanelOpacity = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
    TMap<EItemRarity, FLinearColor> RarityColors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
    TSoftObjectPtr<UMaterialInterface> PanelMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Font")
    FSlateFontInfo TitleFont;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Font")
    FSlateFontInfo BodyFont;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Font")
    FSlateFontInfo ValueFont;

    FLinearColor GetRarityColor(EItemRarity Rarity) const;
};
