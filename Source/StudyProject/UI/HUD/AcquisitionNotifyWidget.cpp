#include "AcquisitionNotifyWidget.h"

void UAcquisitionNotifyWidget::Init(UTexture2D* Icon, const FText& ItemName, EItemRarity Rarity)
{
    OnInit(Icon, ItemName, Rarity);
    PlayNotifyAnimation();
}
