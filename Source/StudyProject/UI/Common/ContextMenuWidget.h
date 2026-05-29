#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "ContextMenuWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnContextAction);

UCLASS(Abstract)
class STUDYPROJECT_API UContextMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowForItem(int32 InSlotIndex, const FItemData& Data, FVector2D ScreenPos);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void Close();

    UPROPERTY(BlueprintAssignable, Category = "UI") FOnContextAction OnEquipClicked;
    UPROPERTY(BlueprintAssignable, Category = "UI") FOnContextAction OnUseClicked;
    UPROPERTY(BlueprintAssignable, Category = "UI") FOnContextAction OnSplitClicked;
    UPROPERTY(BlueprintAssignable, Category = "UI") FOnContextAction OnDropClicked;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    int32 TargetSlotIndex = -1;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnShow(int32 InSlotIndex, const FItemData& Data, FVector2D ScreenPos);
};
