#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/ItemData.h"
#include "CharacterBase.generated.h"

class UInventoryComponent;
class UEquipmentComponent;
class UEnhanceComponent;

UCLASS()
class STUDYPROJECT_API ACharacterBase : public ACharacter
{
    GENERATED_BODY()

public:
    ACharacterBase();

    // 컴포넌트 접근자
    UFUNCTION(BlueprintCallable, Category = "Components")
    UInventoryComponent* GetInventoryComponent() const { return InventoryComp; }

    UFUNCTION(BlueprintCallable, Category = "Components")
    UEquipmentComponent* GetEquipmentComponent() const { return EquipmentComp; }

    UFUNCTION(BlueprintCallable, Category = "Components")
    UEnhanceComponent*   GetEnhanceComponent()   const { return EnhanceComp; }

    // 골드
    UFUNCTION(BlueprintCallable, Category = "Gold")
    int32 GetGold() const { return Gold; }

    UFUNCTION(BlueprintCallable, Category = "Gold")
    bool SpendGold(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Gold")
    void AddGold(int32 Amount);

    // 세이브/로드
    void SaveCharacter();
    void LoadCharacter();

    // Modular Character 슬롯별 메시 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<USkeletalMeshComponent> HeadMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<USkeletalMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<USkeletalMeshComponent> HandsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<USkeletalMeshComponent> LegsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<USkeletalMeshComponent> FeetMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<USkeletalMeshComponent> ShoulderMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<USkeletalMeshComponent> ArmsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<USkeletalMeshComponent> ShieldMesh;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UInventoryComponent> InventoryComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UEquipmentComponent> EquipmentComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UEnhanceComponent> EnhanceComp;

    UPROPERTY(EditDefaultsOnly, Category = "Save")
    FString SaveSlotName = TEXT("PlayerSave");

    UPROPERTY(ReplicatedUsing = OnRep_Gold, BlueprintReadOnly, Category = "Gold")
    int32 Gold = 0;

    UFUNCTION()
    void OnRep_Gold();

private:
    void SetupModularMesh(USkeletalMeshComponent* Part);
};
