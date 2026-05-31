#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShopComponent.generated.h"

class UItemSubsystem;
class UInventoryComponent;
class ACharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopChanged);

/**
 * 서버 권위 상점 컴포넌트 (플레이어에 부착).
 * - 상점 목록 = 기본 목록(DT_ShopInventory) + 랜덤 아이템(DT_ItemData)
 * - 목록/되사기 목록은 서버에서 생성·복제
 * - 구매/판매/되사기는 Server RPC로 검증
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class STUDYPROJECT_API UShopComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UShopComponent();

    // 상점 열기(목록 생성). 클라면 서버에 요청.
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void RequestOpenShop(int32 ShopID);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    void RequestBuy(int32 ShopIndex);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    void RequestSell(int32 InvSlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    void RequestBuyback(int32 BuybackIndex);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    const TArray<int32>& GetShopItems() const { return ShopItemIDs; }

    UFUNCTION(BlueprintCallable, Category = "Shop")
    const TArray<int32>& GetBuybackItems() const { return BuybackItemIDs; }

    // 목록/되사기 갱신 시 브로드캐스트(UI 새로고침용)
    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnShopChanged OnShopChanged;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    int32 RandomItemCount = 6;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    int32 MaxBuyback = 12;

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing = OnRep_Shop)
    TArray<int32> ShopItemIDs;

    UPROPERTY(ReplicatedUsing = OnRep_Shop)
    TArray<int32> BuybackItemIDs;

    UFUNCTION()
    void OnRep_Shop();

    UFUNCTION(Server, Reliable) void Server_OpenShop(int32 ShopID);
    UFUNCTION(Server, Reliable) void Server_Buy(int32 ShopIndex);
    UFUNCTION(Server, Reliable) void Server_Sell(int32 InvSlotIndex);
    UFUNCTION(Server, Reliable) void Server_Buyback(int32 BuybackIndex);

private:
    int32 ActiveShopID = -1;

    void OpenShop_Internal(int32 ShopID);
    void Buy_Internal(int32 ShopIndex);
    void Sell_Internal(int32 InvSlotIndex);
    void Buyback_Internal(int32 BuybackIndex);

    void NotifyChanged();   // 서버에서 직접 호출(스탠드얼론/리슨), 클라는 OnRep_Shop

    UItemSubsystem*     GetItemSub() const;
    UInventoryComponent* GetInv() const;
    ACharacterBase*     GetOwnerChar() const;
};
