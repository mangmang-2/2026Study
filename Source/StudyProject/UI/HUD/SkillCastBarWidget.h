#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillCastBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class USizeBox;
class USkillManagerComponent;

/**
 * 시전(캐스트) 바(코드 전용). 화면 중앙 하단에 시전 중에만 표시.
 * NativeTick에서 SkillManagerComponent의 시전 진행률을 폴링해 채운다.
 */
UCLASS()
class STUDYPROJECT_API USkillCastBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitCastBar(USkillManagerComponent* InComp);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

private:
    UPROPERTY()
    TObjectPtr<UProgressBar> Bar = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> Label = nullptr;

    UPROPERTY()
    TObjectPtr<USizeBox> RootBox = nullptr;

    TWeakObjectPtr<USkillManagerComponent> SkillComp;
};
