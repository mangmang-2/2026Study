#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NPCMarkerWidget.generated.h"

class UImage;

UENUM(BlueprintType)
enum class ENPCMarkerType : uint8
{
    None       UMETA(DisplayName = "없음"),
    Quest      UMETA(DisplayName = "!  퀘스트"),
    QuestDone  UMETA(DisplayName = "? 퀘스트 완료"),
    Shop       UMETA(DisplayName = "상점"),
};

UCLASS(Abstract)
class STUDYPROJECT_API UNPCMarkerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "NPC")
    void SetMarkerType(ENPCMarkerType Type);

    UFUNCTION(BlueprintPure, Category = "NPC")
    ENPCMarkerType GetMarkerType() const { return CurrentType; }

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> MarkerIcon = nullptr;

    // 각 마커 타입별 아이콘 — BP에서 설정
    UPROPERTY(EditDefaultsOnly, Category = "NPC|Marker")
    TObjectPtr<UTexture2D> QuestIcon = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "NPC|Marker")
    TObjectPtr<UTexture2D> QuestDoneIcon = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "NPC|Marker")
    TObjectPtr<UTexture2D> ShopIcon = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "NPC")
    void OnMarkerTypeChanged(ENPCMarkerType NewType);

private:
    ENPCMarkerType CurrentType = ENPCMarkerType::None;
};
