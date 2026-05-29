#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/GameData.h"
#include "EnhanceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnhanceResult, bool, bSuccess, int32 , NewEnhanceLevel);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STUDYPROJECT_API UEnhanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnhanceComponent();

    // 강화 시도 (서버 권위)
    UFUNCTION(BlueprintCallable, Category = "Enhance")
    void TryEnhance(int32 InventorySlot);

    UPROPERTY(BlueprintAssignable, Category = "Enhance")
    FOnEnhanceResult OnEnhanceResult;

    static const int32 MaxEnhanceLevel = 10;

    const FEnhanceRateRow* GetEnhanceRate(int32 Level) const;

    // 강화 결과 이펙트 — BP에서 Niagara 등 시각 효과 구현
    UFUNCTION(BlueprintImplementableEvent, Category = "Enhance")
    void OnEnhanceVisualEffect(bool bSuccess);

private:
    UFUNCTION(Server, Reliable)
    void Server_TryEnhance(int32 InventorySlot);

    void Internal_TryEnhance(int32 InventorySlot);

    // 모든 클라이언트에 강화 결과 브로드캐스트
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnEnhanceResult(bool bSuccess, int32 NewLevel);

    UPROPERTY()
    TObjectPtr<UDataTable> EnhanceRateTable = nullptr;
};
