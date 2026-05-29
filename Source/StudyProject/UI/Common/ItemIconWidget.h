#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "ItemIconWidget.generated.h"

class UImage;
class UBorder;

UCLASS(Abstract)
class STUDYPROJECT_API UItemIconWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Icon")
    void SetItemData(const FItemData& Data);

    UFUNCTION(BlueprintCallable, Category = "Icon")
    void SetIconTexture(UTexture2D* Texture);

    UFUNCTION(BlueprintCallable, Category = "Icon")
    void SetRarity(EItemRarity Rarity);

    UFUNCTION(BlueprintCallable, Category = "Icon")
    void Clear();

    static FLinearColor GetRarityColor(EItemRarity Rarity);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> IconImage = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> RarityBorder = nullptr;
};
