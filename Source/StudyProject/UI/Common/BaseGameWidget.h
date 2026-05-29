#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "BaseGameWidget.generated.h"

class UUIStyleDataAsset;

UCLASS(Abstract)
class STUDYPROJECT_API UBaseGameWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ApplyStyle();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSoftObjectPtr<UUIStyleDataAsset> StyleAsset;

    virtual void NativeConstruct() override;
};
