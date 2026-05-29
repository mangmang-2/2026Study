#include "LockOnMarkerWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"

void ULockOnMarkerWidget::SetWorldTarget(AActor* Target)
{
    TrackedTarget = Target;
    SetVisibility(Target ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void ULockOnMarkerWidget::UpdatePosition(FVector WorldPos)
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    FVector2D ScreenPos;
    bool bOnScreen = PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos);
    if (bOnScreen)
    {
        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
        if (CanvasSlot) CanvasSlot->SetPosition(ScreenPos);
        SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ULockOnMarkerWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);

    if (!TrackedTarget) return;

    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    FVector2D ScreenPos;
    bool bOnScreen = PC->ProjectWorldLocationToScreen(TrackedTarget->GetActorLocation(), ScreenPos);

    if (bOnScreen)
    {
        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
        if (CanvasSlot) CanvasSlot->SetPosition(ScreenPos);
        SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
}
