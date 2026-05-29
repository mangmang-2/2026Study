#include "NPCMarkerWidget.h"
#include "Components/Image.h"

void UNPCMarkerWidget::SetMarkerType(ENPCMarkerType Type)
{
    if (CurrentType == Type) return;
    CurrentType = Type;

    if (MarkerIcon)
    {
        UTexture2D* Icon = nullptr;
        switch (Type)
        {
        case ENPCMarkerType::Quest:     Icon = QuestIcon;     break;
        case ENPCMarkerType::QuestDone: Icon = QuestDoneIcon; break;
        case ENPCMarkerType::Shop:      Icon = ShopIcon;      break;
        default: break;
        }

        if (Icon)
        {
            MarkerIcon->SetBrushFromTexture(Icon);
            MarkerIcon->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            MarkerIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    OnMarkerTypeChanged(Type);
}
