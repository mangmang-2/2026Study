#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "AcquisitionNotifyWidget.generated.h"

UCLASS(Abstract)
class STUDYPROJECT_API UAcquisitionNotifyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void Init(UTexture2D* Icon, const FText& ItemName, EItemRarity Rarity);

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlayNotifyAnimation();

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnInit(UTexture2D* Icon, const FText& ItemName, EItemRarity Rarity);
};
