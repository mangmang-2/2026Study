#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillHUDWidget.generated.h"

class UImage;
class UBorder;
class UTextBlock;
class UHorizontalBox;
class USkillManagerComponent;

/**
 * 스킬 슬롯 HUD(코드 전용 — WBP 불필요). 화면 하단 중앙에 Q/E/R 3슬롯.
 * NativeTick에서 SkillManagerComponent를 폴링해 아이콘/쿨다운(어둡게+카운트다운)을 실시간 갱신.
 */
UCLASS()
class STUDYPROJECT_API USkillHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitHUD(USkillManagerComponent* InComp);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

private:
    static constexpr int32 NumSlots = 3;

    void BuildSlot(int32 Index, UHorizontalBox* Row);

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> SlotBgBorders;

    UPROPERTY()
    TArray<TObjectPtr<UImage>> IconImages;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> CooldownOverlays;

    UPROPERTY()
    TArray<TObjectPtr<UTextBlock>> CooldownTexts;

    TWeakObjectPtr<USkillManagerComponent> SkillComp;

    // 슬롯 배경/빈 슬롯 색
    UPROPERTY(EditAnywhere, Category = "Style")
    FLinearColor EmptySlotColor = FLinearColor(0.06f, 0.06f, 0.09f, 0.85f);

    UPROPERTY(EditAnywhere, Category = "Style")
    FLinearColor FilledSlotColor = FLinearColor(0.18f, 0.20f, 0.30f, 0.95f);

    UPROPERTY(EditAnywhere, Category = "Style")
    FLinearColor CooldownTint = FLinearColor(0.f, 0.f, 0.f, 0.65f);
};
