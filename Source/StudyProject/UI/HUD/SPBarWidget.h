#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SPBarWidget.generated.h"

class UProgressBar;

/**
 * 플레이어 SP(에너지) / HP 바 위젯.
 * 매 프레임 소유 폰의 ASC에서 SP·HP를 읽어 ProgressBar를 갱신한다.
 * WBP에서 "SPBar"(필수) / "HPBar"(선택) 이름의 ProgressBar를 바인딩.
 */
UCLASS()
class STUDYPROJECT_API USPBarWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> SPBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> HPBar;
};
