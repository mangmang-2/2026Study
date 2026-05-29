#include "BaseGameWidget.h"
#include "UI/Common/UIStyleDataAsset.h"

void UBaseGameWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ApplyStyle();
}

void UBaseGameWidget::ApplyStyle()
{
    // 서브클래스에서 StyleAsset 로드 후 색상/폰트 적용
}
