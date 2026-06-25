#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillTreeEntryWidget.generated.h"

class UTextBlock;
class UButton;
class USkillDefinition;

// 스킬을 슬롯에 배정 요청 (풀 인덱스, 슬롯 0/1/2)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillAssignRequested, int32, PoolIndex, int32, InSlot);

/**
 * 스킬트리 한 행(코드 전용): [스킬 이름] [Z][X][C] 배정 버튼.
 */
UCLASS()
class STUDYPROJECT_API USkillTreeEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitEntry(int32 InPoolIndex, USkillDefinition* InSkill);

    UPROPERTY(BlueprintAssignable)
    FOnSkillAssignRequested OnAssignRequested;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UFUNCTION()
    void OnSlot0();
    UFUNCTION()
    void OnSlot1();
    UFUNCTION()
    void OnSlot2();

    UButton* MakeSlotButton(const FString& Label);

    // NameText/CachedSkill 준비되면 이름 적용(InitEntry·RebuildWidget 순서 무관 대응)
    void ApplyName();

    UPROPERTY()
    TObjectPtr<UTextBlock> NameText = nullptr;

    UPROPERTY()
    TObjectPtr<USkillDefinition> CachedSkill = nullptr;

    int32 PoolIndex = -1;
};
