#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/ItemData.h"
#include "ItemBase.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UInventoryComponent;

UCLASS()
class STUDYPROJECT_API AItemBase : public AActor
{
    GENERATED_BODY()

public:
    AItemBase();

    void InitItem(int32 InItemID, int32 InQuantity);

    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetItemID()   const { return ItemID; }
    int32 GetQuantity() const { return Quantity; }

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USphereComponent> PickupCollision;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> MeshComponent;

    UPROPERTY(EditDefaultsOnly, Category = "Item")
    float PickupRadius = 100.f;

    UPROPERTY(ReplicatedUsing = OnRep_ItemID, VisibleAnywhere, Category = "Item")
    int32 ItemID = 0;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
    int32 Quantity = 1;

private:
    UFUNCTION()
    void OnRep_ItemID();

    void ApplyMesh();

    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION(Server, Reliable)
    void Server_PickUp(ACharacter* Picker);
};
