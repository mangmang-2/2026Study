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
class UStaticMeshComponent;
class UNiagaraComponent;

UCLASS()
class STUDYPROJECT_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ACharacterBase();

    // IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // 타격 피드백(VFX+플래시+데미지넘버)을 전 클라에 재생. 서버 권위에서만 호출.
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_HitFeedback(class UNiagaraSystem* VFX, FVector Location, FVector Normal,
        AActor* Victim, int32 Damage, bool bCritical);

    // 데미지 숫자만 전 클라에 표시(VFX/플래시 없음 — DOT 틱용). 서버에서만 호출.
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_DamageNumber(FVector Location, int32 Damage, EDamageType Type);

    // 사망 시 래그돌 전환을 전 클라에 적용. 물리는 복제 안 되므로 멀티캐스트 필수. 서버에서만 호출.
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_EnterRagdoll();

    // 래그돌 해제(리스폰 등) — 메시 캡슐 재부착·물리 정지를 전 클라에 적용. 서버에서만 호출.
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_ExitRagdoll();

    // 어떤 ACharacter든 래그돌 물리로 전환하는 공용 로직(멀티캐스트에서 호출).
    // 적(AEnemyCharacter)은 ACharacterBase가 아니라 이 헬퍼를 공유한다.
    static void ApplyRagdollPhysics(class ACharacter* Char);

protected:
    // 피격 시 피격자 메시에 잠깐 씌우는 흰색 플래시 오버레이 머티리얼
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|FX")
    TObjectPtr<class UMaterialInterface> HitFlashMaterial;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|FX")
    float HitFlashDuration = 0.08f;

public:

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

    // 기준 걷기 속도 변경 — 둔화/스턴 반영해 MaxWalkSpeed 즉시 적용
    void SetBaseWalkSpeed(float NewBaseSpeed);

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

    // 스태틱 메시 무기용(스태틱 무기 장착 시 사용)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<UStaticMeshComponent> WeaponStaticMesh;

    // 강화 무기 오라 VFX(장착 무기에 부착)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    TObjectPtr<UNiagaraComponent> WeaponAuraVFX;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

    // SP 자동 회복 GE (무한 주기 GE — 공격 중엔 State.Attacking으로 정지)
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TSubclassOf<UGameplayEffect> SPRegenEffect;

    // ASC ActorInfo 초기화 + (서버) 기본 어빌리티/스탯 부여
    void InitAbilitySystem();
    bool bAbilitiesGranted = false;

    // ── 이동속도(상태이상 둔화/스턴 반영) ────────────────────────────
    // 평상시 기준 속도. RefreshMoveSpeed가 둔화/스턴 태그에 따라 MaxWalkSpeed 재계산.
    float BaseWalkSpeed = 0.f;

    // Status.Chilled일 때 BaseWalkSpeed에 곱하는 둔화 배율
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Status")
    float ChillSpeedFactor = 0.45f;

    UFUNCTION()
    void OnMoveStatusTagChanged(const FGameplayTag Tag, int32 NewCount);

    void RefreshMoveSpeed();

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

    // 래그돌 해제 시 메인 메시를 캡슐에 되돌리기 위한 기본값(BeginPlay에서 캡처)
    FTransform DefaultMeshRelativeTransform;
    FName      DefaultMeshCollisionProfile;
};
