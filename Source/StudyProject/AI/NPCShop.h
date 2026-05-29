#pragma once

#include "CoreMinimal.h"
#include "AI/AICharacterBase.h"
#include "NPCShop.generated.h"

class UInventoryComponent;
class USphereComponent;

UCLASS()
class STUDYPROJECT_API ANPCShop : public AAICharacterBase
{
    GENERATED_BODY()

public:
    ANPCShop();

    // 플레이어가 상점 범위 진입 시 호출 — 상점 창 열기 요청
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void OpenShopFor(ACharacter* Customer);

    // 구매: 서버 검증 → 골드 차감 + 아이템 지급
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void BuyItem(ACharacter* Customer, int32 ItemID, int32 Quantity);

    // 판매: 서버 검증 → 아이템 제거 + 골드 지급
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void SellItem(ACharacter* Customer, int32 InventorySlot, int32 Quantity);

    UFUNCTION(BlueprintPure, Category = "Shop")
    TArray<int32> GetShopItemList() const;

protected:
    virtual void BeginPlay() override;

    // 상호작용 콜리전
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
    TObjectPtr<USphereComponent> InteractSphere;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
    float InteractRadius = 200.f;

private:
    UFUNCTION()
    void OnInteractBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnInteractEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UFUNCTION(Server, Reliable)
    void Server_BuyItem(ACharacter* Customer, int32 ItemID, int32 Quantity);

    UFUNCTION(Server, Reliable)
    void Server_SellItem(ACharacter* Customer, int32 InventorySlot, int32 Quantity);

    // 골드 조회/차감 헬퍼 (CharacterBase에 Gold 변수 추가 전 임시 스텁)
    int32 GetCustomerGold(ACharacter* Customer) const;
    bool  DeductGold(ACharacter* Customer, int32 Amount);
    void  AddGold(ACharacter* Customer, int32 Amount);
};
