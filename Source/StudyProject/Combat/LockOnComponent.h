#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

/**
 * 록온 컴포넌트(플레이어).
 * 토글로 시야 내 가장 가까운 적을 타겟으로 지정하고, 락 중에는 카메라/캐릭터가
 * 타겟을 바라보게 한다. 타겟이 죽거나 멀어지면 자동 해제. 좌우 전환 지원.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class STUDYPROJECT_API ULockOnComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULockOnComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void ToggleLockOn();

    // Direction < 0 : 왼쪽 타겟, > 0 : 오른쪽 타겟으로 전환
    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void SwitchTarget(float Direction);

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    bool IsLockedOn() const { return CurrentTarget.IsValid(); }

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

protected:
    UPROPERTY(EditDefaultsOnly, Category = "LockOn")
    float SearchRange = 1200.f;

    UPROPERTY(EditDefaultsOnly, Category = "LockOn")
    float LoseRange = 1600.f;

    UPROPERTY(EditDefaultsOnly, Category = "LockOn")
    float RotationInterpSpeed = 10.f;

    // 타겟으로 인정할 최소 시야 내적(카메라 정면과의 dot). 0=정면 90도까지
    UPROPERTY(EditDefaultsOnly, Category = "LockOn")
    float MinViewDot = 0.f;

private:
    TWeakObjectPtr<AActor> CurrentTarget;

    AActor* FindBestTarget(float SideSign) const;
    bool IsValidTarget(AActor* Target) const;
    void StartLockOn(AActor* Target);
    void StopLockOn();
};
