#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InteractionDetectorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusChanged, AActor*, NewFocus);

// 플레이어에 부착 — 범위 내 IInteractable 감지 + 가장 가까운 대상 추적
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STUDYPROJECT_API UInteractionDetectorComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    UInteractionDetectorComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // F키 입력 시 호출
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void TryInteract();

    UFUNCTION(BlueprintPure, Category = "Interaction")
    AActor* GetFocusedActor() const { return FocusedActor.Get(); }

    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnFocusChanged OnFocusChanged;

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    void UpdateFocus();

    TArray<TWeakObjectPtr<AActor>> Candidates;
    TWeakObjectPtr<AActor>         FocusedActor;
};
