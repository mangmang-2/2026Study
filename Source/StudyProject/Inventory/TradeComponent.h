#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/ItemData.h"
#include "TradeComponent.generated.h"

USTRUCT(BlueprintType)
struct FTradeSlot
{
    GENERATED_BODY()

    UPROPERTY() 
    int32 ItemID    = 0;
    UPROPERTY() 
    int32 Quantity  = 0;
    UPROPERTY() 
    int32 SlotIndex = -1;
};

USTRUCT(BlueprintType)
struct FTradeOffer
{
    GENERATED_BODY()

    UPROPERTY() 
    TArray<FTradeSlot> Slots;
    UPROPERTY() 
    bool bConfirmed = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTradeUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeResult, bool, bSuccess);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STUDYPROJECT_API UTradeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTradeComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void RequestTrade(ACharacter* TargetPlayer);

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void RegisterItem(int32 InvSlotIndex, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void UnregisterItem(int32 TradeSlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void ConfirmTrade();

    UFUNCTION(BlueprintCallable, Category = "Trade")
    void CancelTrade();

    UFUNCTION(BlueprintCallable, Category = "Trade")
    bool IsTrading() const { return PartnerActor != nullptr; }

    UFUNCTION(BlueprintCallable, Category = "Trade")
    const FTradeOffer& GetMyOffer()      const { return MyOffer; }

    UFUNCTION(BlueprintCallable, Category = "Trade")
    const FTradeOffer& GetPartnerOffer() const { return PartnerOffer; }

    UPROPERTY(BlueprintAssignable, Category = "Trade")
    FOnTradeUpdated OnTradeUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Trade")
    FOnTradeResult OnTradeResult;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_MyOffer)
    FTradeOffer MyOffer;

    UPROPERTY(ReplicatedUsing = OnRep_PartnerOffer)
    FTradeOffer PartnerOffer;

    UPROPERTY(ReplicatedUsing = OnRep_PartnerActor)
    TObjectPtr<AActor> PartnerActor = nullptr;

private:
    UFUNCTION()
    void OnRep_MyOffer();

    UFUNCTION()
    void OnRep_PartnerOffer();

    UFUNCTION()
    void OnRep_PartnerActor();

    UFUNCTION(Server, Reliable)
    void Server_RequestTrade(ACharacter* Target);

    UFUNCTION(Server, Reliable)
    void Server_RegisterItem(int32 InvSlotIndex, int32 Quantity);

    UFUNCTION(Server, Reliable)
    void Server_UnregisterItem(int32 TradeSlotIndex);

    UFUNCTION(Server, Reliable)
    void Server_ConfirmTrade();

    UFUNCTION(Server, Reliable)
    void Server_CancelTrade();

    void Internal_ExecuteTrade();
};
