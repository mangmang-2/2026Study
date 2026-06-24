#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillTreeWidget.generated.h"

class UVerticalBox;
class UScrollBox;
class UTextBlock;
class USkillManagerComponent;

/**
 * 스킬트리 패널(코드 전용): 보유 스킬 목록을 행으로 보여주고 Z/X/C 슬롯에 배정.
 * 화면 중앙 팝업. 행별 [이름][Z][X][C] 버튼 → SkillManagerComponent.AssignSkill.
 */
UCLASS()
class STUDYPROJECT_API USkillTreeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitTree(USkillManagerComponent* InComp);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void RefreshList();

    UFUNCTION()
    void HandleAssign(int32 PoolIndex, int32 InSlot);

    UFUNCTION()
    void HandleSlotsChanged();

    UPROPERTY()
    TObjectPtr<UScrollBox> ListBox = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> SlotsText = nullptr;

    TWeakObjectPtr<USkillManagerComponent> SkillComp;
};
