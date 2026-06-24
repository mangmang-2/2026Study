#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillCastBarWorldWidget.generated.h"

class UProgressBar;
class USkillManagerComponent;

/**
 * 머리 위(WorldSpace) 캐스트 바(코드 전용). 다른 플레이어가 시전 중인 걸 보여준다.
 * SkillManagerComponent의 멀티캐스트로 복제된 월드 캐스트 상태를 폴링.
 */
UCLASS()
class STUDYPROJECT_API USkillCastBarWorldWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitWorldCastBar(USkillManagerComponent* InComp);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

private:
    UPROPERTY()
    TObjectPtr<UProgressBar> Bar = nullptr;

    TWeakObjectPtr<USkillManagerComponent> SkillComp;
};
