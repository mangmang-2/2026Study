#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LockOnMarkerWidget.generated.h"

UCLASS(Abstract)
class STUDYPROJECT_API ULockOnMarkerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetWorldTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdatePosition(FVector WorldPos);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    TObjectPtr<AActor> TrackedTarget = nullptr;

    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
};
