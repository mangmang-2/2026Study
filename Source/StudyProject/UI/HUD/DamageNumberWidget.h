#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "DamageNumberWidget.generated.h"

UCLASS(Abstract)
class STUDYPROJECT_API UDamageNumberWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void Init(int32 Damage, EDamageType Type, FVector WorldPos);

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnInit(int32 Damage, EDamageType Type, FVector WorldPos);

    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void PlayFloatAnimation();
};
