#include "DamageNumberWidget.h"

void UDamageNumberWidget::Init(int32 Damage, EDamageType Type, FVector WorldPos)
{
    OnInit(Damage, Type, WorldPos);
    PlayFloatAnimation();
}
