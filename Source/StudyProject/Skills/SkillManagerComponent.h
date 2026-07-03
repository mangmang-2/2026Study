#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "SkillManagerComponent.generated.h"

class USkillDefinition;
class UGA_SkillExecutor;
class UDecalComponent;
class UMaterialInterface;

// 슬롯 구성 변경(배정/해제) — UI 갱신용
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillSlotsChanged);

// 슬롯 쿨다운 시작 — UI sweep 시작용 (슬롯 인덱스, 총 쿨다운 초)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillCooldownStarted, int32, SlotIndex, float, Duration);

/**
 * 스킬 보유/슬롯 배정/쿨다운을 관리하는 컴포넌트(PlayerCharacter 부착).
 * UI와 어빌리티 사이의 단일 진실 출처.
 *
 * - EquippedSkills: Q/E/R 3슬롯(복제). 스킬트리 UI가 배정.
 * - 발동: 소유 클라가 타겟팅 해석 → Server_ActivateSlot RPC → 서버가 GA 실행.
 * - 쿨다운: 소유 클라 로컬(예측·UI) + 서버 권위 게이팅. 월드 시간 기준.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class STUDYPROJECT_API USkillManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USkillManagerComponent();

    static constexpr int32 NumSlots = 3;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ── 입력 진입점(소유 클라) ──────────────────────────────────────
    // 키 Pressed — PointTarget이면 타겟팅 프리뷰 시작(커서+데칼), 그 외엔 즉시 발동.
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void ActivateSlot(int32 SlotIndex);

    // 키 Released — 타겟팅 중이던 슬롯이면 현재 조준 위치로 발동.
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void ReleaseSlot(int32 SlotIndex);

    // ── 슬롯 배정(스킬트리 UI) ──────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void AssignSkill(USkillDefinition* Skill, int32 SlotIndex);

    // ── UI 조회 ─────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Skills")
    USkillDefinition* GetSlotSkill(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Skills")
    float GetCooldownRemaining(int32 SlotIndex) const;

    // 0(준비됨) ~ 1(방금 시작)
    UFUNCTION(BlueprintCallable, Category = "Skills")
    float GetCooldownFraction(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Skills")
    TArray<USkillDefinition*> GetSkillPool() const;

    // ── 시전(캐스트바) 상태 — 소유 클라 로컬 예측 ──────────────────
    UFUNCTION(BlueprintCallable, Category = "Skills")
    bool IsCasting() const;

    // 0(시작) ~ 1(완료)
    UFUNCTION(BlueprintCallable, Category = "Skills")
    float GetCastProgress() const;

    UFUNCTION(BlueprintCallable, Category = "Skills")
    USkillDefinition* GetCastingSkill() const;

    // ── 월드(머리 위) 캐스트 — 모든 클라(타 플레이어 시전 표시) ──────
    UFUNCTION(BlueprintCallable, Category = "Skills")
    bool IsWorldCasting() const;

    UFUNCTION(BlueprintCallable, Category = "Skills")
    float GetWorldCastProgress() const;

    // GA_SkillExecutor가 활성화 시 호출 — 서버가 쌓아둔 스킬·타겟을 가져가며 비운다.
    // OutTarget = 호밍 대상(락온 시), 없으면 nullptr.
    USkillDefinition* ConsumePendingActivation(FVector& OutOrigin, FVector& OutDirection, AActor*& OutTarget);

    UPROPERTY(BlueprintAssignable, Category = "Skills")
    FOnSkillSlotsChanged OnSlotsChanged;

    UPROPERTY(BlueprintAssignable, Category = "Skills")
    FOnSkillCooldownStarted OnCooldownStarted;

protected:
    // 시작 시 슬롯에 채워둘 스킬 풀(기획자 편집). 앞에서부터 NumSlots개를 슬롯에 배정.
    UPROPERTY(EditAnywhere, Category = "Skills")
    TArray<TObjectPtr<USkillDefinition>> DefaultSkills;

    // 실행에 쓸 GA 클래스(없으면 UGA_SkillExecutor)
    UPROPERTY(EditAnywhere, Category = "Skills")
    TSubclassOf<UGA_SkillExecutor> ExecutorAbilityClass;

    UPROPERTY(ReplicatedUsing = OnRep_EquippedSkills)
    TArray<TObjectPtr<USkillDefinition>> EquippedSkills;

private:
    UFUNCTION()
    void OnRep_EquippedSkills();

    UFUNCTION(Server, Reliable)
    void Server_ActivateSlot(int32 SlotIndex, FVector Origin, FVector Direction, AActor* TargetActor);

    UFUNCTION(Server, Reliable)
    void Server_AssignSkill(int32 PoolIndex, int32 SlotIndex);

    // 서버가 발동 성공 시 소유 클라 UI 쿨다운을 시작시킨다(원격 클라 예측 표시).
    UFUNCTION(Client, Reliable)
    void Client_StartCooldown(int32 SlotIndex, float Duration);

    // 서버가 시전 시작 시 전 클라에 알림 → 각 클라가 머리 위 캐스트바를 로컬 타이머로 채움.
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_CastStarted(float Duration);

    // 슬롯 발동 본체(타겟 해석 → 시전 예측 → 서버 RPC)
    void FireSlot(int32 SlotIndex);

    // 타겟팅 프리뷰 시작/종료(소유 클라, PointTarget 전용)
    void BeginTargeting(int32 SlotIndex);
    void EndTargeting(bool bFire);

    // 소유 클라에서 스킬 종류별 타겟(Origin/Direction) 해석. 실패 시 false.
    bool ResolveTargeting(USkillDefinition* Skill, FVector& OutOrigin, FVector& OutDirection) const;

    // 락온 중이면 락온 대상 반환(호밍 투사체용), 아니면 nullptr.
    AActor* ResolveHomingTarget() const;

    // 마우스 커서(없으면 카메라 정면)가 가리키는 지면 지점. 프리뷰/조준 공용.
    bool GetGroundAimPoint(FVector& OutPoint) const;

    // 서버에서 GA 부여 보장(최초 1회)
    void EnsureExecutorGranted();

    // 쿨다운 시작(월드 시간 기준). 클라/서버 각자 자기 시계로 기록.
    void StartCooldown(int32 SlotIndex, float Duration);

    class UAbilitySystemComponent* GetOwnerASC() const;

    // 슬롯별 쿨다운 종료 월드시각(로컬). 0이면 준비됨.
    TArray<float> CooldownEndTime;
    // 슬롯별 마지막 쿨다운 총 길이(Fraction 계산용)
    TArray<float> CooldownDuration;

    FGameplayAbilitySpecHandle ExecutorHandle;
    bool bExecutorGranted = false;

    // 서버가 GA 활성화 직전 쌓아두는 실행 인자(GA가 ConsumePendingActivation으로 가져감)
    UPROPERTY()
    TObjectPtr<USkillDefinition> PendingSkill = nullptr;
    FVector PendingOrigin = FVector::ZeroVector;
    FVector PendingDirection = FVector::ForwardVector;

    // 호밍 투사체 대상(락온 시). GA가 ConsumePendingActivation으로 가져감.
    UPROPERTY()
    TObjectPtr<AActor> PendingTarget = nullptr;

    // 시전바용 로컬 캐스트 상태(소유 클라 예측)
    UPROPERTY()
    TObjectPtr<USkillDefinition> CastingSkill = nullptr;
    float CastStartTime = 0.f;
    float CastEndTime = 0.f;

    // 월드(머리 위) 캐스트 — 멀티캐스트로 각 클라가 로컬 채움
    float WorldCastStart = 0.f;
    float WorldCastEnd = 0.f;

    UPROPERTY()
    TObjectPtr<class UWidgetComponent> CastBarWorldComp = nullptr;

    UPROPERTY(EditAnywhere, Category = "Skills")
    TSubclassOf<class USkillCastBarWorldWidget> WorldCastBarWidgetClass;

    // ── 타겟팅 프리뷰(소유 클라 로컬) ────────────────────────────────
    bool bTargeting = false;
    int32 TargetingSlot = -1;

    UPROPERTY()
    TObjectPtr<UDecalComponent> PreviewDecal = nullptr;

    // 스킬에 RangeDecalMaterial이 없을 때 쓸 기본 데칼 머티리얼
    UPROPERTY(EditAnywhere, Category = "Skills")
    TObjectPtr<UMaterialInterface> DefaultRangeDecalMaterial = nullptr;
};
