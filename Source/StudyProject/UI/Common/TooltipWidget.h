#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "TooltipWidget.generated.h"

class UTextBlock;

UCLASS(Abstract)
class STUDYPROJECT_API UTooltipWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetItemData(const FItemData& Data);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetPosition(FVector2D ScreenPos);

protected:
    // WBP_TooltipWidget 내부 텍스트 블록 (BP 이벤트 없이 C++에서 직접 채움)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ItemNameText    = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StatsText       = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DescriptionText = nullptr;

    // BP에서 추가 연출을 하고 싶을 때만 구현(선택)
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnItemDataSet(const FItemData& Data);
};
