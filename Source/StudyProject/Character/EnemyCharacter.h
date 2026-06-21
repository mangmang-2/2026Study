#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "EnemyCharacter.generated.h"

class UCombatAbilitySystemComponent;
class UCombatAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class USkeletalMeshComponent;
class USkeletalMesh;
class UMaterialInterface;
class UDecalComponent;

/**
 * GAS 기반 테스트용 적 캐릭터.
 * 자체 ASC + AttributeSet를 갖고, 피격/사망/넉다운 등 반응 어빌리티를
 * Event.* 게임플레이 이벤트로 트리거한다. (플레이어 콤보가 GE_Damage를 적용)
 */
UCLASS()
class STUDYPROJECT_API AEnemyCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual void PossessedBy(AController* NewController) override;

    // 기준 걷기 속도 변경 — 둔화/스턴 반영해 MaxWalkSpeed 즉시 적용
    void SetBaseWalkSpeed(float NewBaseSpeed);

    // 워닝 데칼을 GrowTime 동안 성장 시작(코스메틱이라 멀티캐스트)
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_StartGrowingDecal(UMaterialInterface* DecalMaterial, FVector Anchor, FVector Dir, float FullLength, float Width, float Depth, float GrowTime);

    // 워닝 데칼 제거(돌진 시작 시 모든 클라 동기 제거)
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_DestroyWarningDecal();

    // 사망 래그돌 전환을 전 클라에 적용(적은 ACharacterBase가 아니라 자체 멀티캐스트 필요). 서버에서만 호출.
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_EnterRagdoll();

    // 전투 간보기(블록) 자세 여부 — ABP가 읽어 애님 레이어를 전환. 서버 BT가 세팅, 복제됨.
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "AI")
    bool bBlockStance = false;

    void SetBlockStance(bool bNew) { bBlockStance = bNew; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

    // ASC ActorInfo 초기화 + (서버) 기본 어빌리티/스탯 부여
    void InitAbilitySystem();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UCombatAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<UCombatAttributeSet> AttributeSet;

    // 이벤트 트리거 기반 반응 어빌리티 (HitReact / Death / AirLaunch 등)
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // 스탯 초기화 GE (없으면 AttributeSet 기본값 사용)
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TSubclassOf<UGameplayEffect> DefaultAttributeEffect;

    // 적 전용 시작 체력(>0이면 기본값 대신 사용)
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float StartingMaxHP = 1000.f;

    // AI가 사용할 공격 어빌리티(서버에서 부여). 기본 GA_EnemyAttack.
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<UGameplayAbility> AttackAbilityClass;

    // 오른손 hand_r 본 무기(플레이어 소켓과 동일 트랜스폼)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TObjectPtr<USkeletalMesh> WeaponMeshAsset;

    // 왼손(hand_l) 방패
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<USkeletalMeshComponent> ShieldMesh;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TObjectPtr<USkeletalMesh> ShieldMeshAsset;

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

    // 상태이상 지속 VFX는 GameplayCue(GCN_StatusAura)가 처리 — GE 적용/만료에 자동 연동.

    // ── 워닝 데칼 성장 ──────────────────────────────────────────────
    void UpdateGrowingDecal();   // 타이머: 데칼 길이/위치를 보스→끝점으로 성장

    UPROPERTY(Transient)
    TObjectPtr<UDecalComponent> WarningDecalComp;

    FVector DecalAnchor = FVector::ZeroVector;   // 성장 기준점(보스 발밑)
    FVector DecalDir = FVector::ForwardVector;   // 돌진 방향
    float DecalFullLength = 0.f;
    float DecalHalfWidth = 0.f;
    float DecalDepth = 0.f;
    float DecalGrowTime = 0.f;
    float DecalGrowElapsed = 0.f;
    FTimerHandle DecalGrowTimer;
};
