#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "Data/ItemData.h"
#include "CharacterBase.generated.h"

class UInventoryComponent;
class UEquipmentComponent;
class UEnhanceComponent;
class UShopComponent;
class UCombatAbilitySystemComponent;
class UCombatAttributeSet;
class UGameplayAbility;
class UGameplayEffect;

UCLASS()
class STUDYPROJECT_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ACharacterBase();

    // IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UCombatAttributeSet* GetCombatAttributeSet() const { return AttributeSet; }

    virtual void PossessedBy(AController* NewController) override;

    // 컴포넌트 접근자
    UFUNCTION(BlueprintCallable, Category = "Components")
    UInventoryComponent* GetInventoryComponent() const { return InventoryComp; }

    UFUNCTION(BlueprintCallable, Category = "Components")
    UEquipmentComponent* GetEquipmentComponent() const { return EquipmentComp; }

    UFUNCTION(BlueprintCallable, Category = "Components")
    UEnhanceComponent*   GetEnhanceComponent()   const { return EnhanceComp; }

    UFUNCTION(BlueprintCallable, Category = "Components")
    UShopComponent*      GetShopComponent()      const { return ShopComp; }

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

    // ── GAS ──────────────────────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UCombatAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<UCombatAttributeSet> AttributeSet;

    // 서버에서 부여할 기본 어빌리티 (콤보/회피 등)
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // 스탯 초기화용 GE (없으면 AttributeSet 기본값 사용)
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TSubclassOf<UGameplayEffect> DefaultAttributeEffect;

    // ASC ActorInfo 초기화 + (서버) 기본 어빌리티/스탯 부여
    void InitAbilitySystem();
    bool bAbilitiesGranted = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UInventoryComponent> InventoryComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UEquipmentComponent> EquipmentComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UEnhanceComponent> EnhanceComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UShopComponent> ShopComp;

    UPROPERTY(EditDefaultsOnly, Category = "Save")
    FString SaveSlotName = TEXT("PlayerSave");

    UPROPERTY(ReplicatedUsing = OnRep_Gold, BlueprintReadOnly, Category = "Gold")
    int32 Gold = 0;

    UFUNCTION()
    void OnRep_Gold();

private:
    void SetupModularMesh(USkeletalMeshComponent* Part);
};
