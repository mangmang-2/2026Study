#pragma once

#include "CoreMinimal.h"
#include "GAS/CombatGameplayAbility.h"
#include "SkillTypes.h"
#include "GA_SkillExecutor.generated.h"

class USkillDefinition;

/**
 * 데이터 기반 스킬 실행 어빌리티.
 * SkillManagerComponent가 PrepareSkill로 스킬·타겟을 주입한 뒤 활성화한다(서버 권위).
 *
 * 흐름: CastMontage 재생 → CastTime 대기 → Detonate(전달방식별 판정) →
 *       EffectModules[] 순회 Execute(동시/순차) → EndAbility.
 * 보스 돌진과 동일하게 ServerOnly + 착탄 VFX는 멀티캐스트.
 */
UCLASS()
class STUDYPROJECT_API UGA_SkillExecutor : public UCombatGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_SkillExecutor();

    // 한 지점에서 즉발 실행(판정 + VFX + 모듈 동시). 투사체 충돌 시 등 외부에서 호출.
    static void ExecuteSkillBurstAt(UWorld* World, USkillDefinition* Skill,
        class UAbilitySystemComponent* InstigatorASC, AActor* Instigator,
        UGameplayAbility* SourceAbility, const FVector& Origin, const FVector& Direction);

    // 후보가 유효 타겟인지(적/사망/래그돌 필터) — 정적 버전
    static bool IsHostileValidTarget(AActor* Candidate, bool bInstigatorIsEnemy);

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

private:
    // CastTime 경과 후 호출 — 단발이면 1회, 지속이면 필드 틱 시작
    void Detonate();

    // 단발 실행(판정 + VFX + 모듈 동시/순차)
    void RunOneShot();

    // 지속(필드) 1틱 — 매 틱 재판정 + VFX 펄스 + 모듈 동시 실행
    void FieldTick();

    // 착탄 VFX 멀티캐스트(범위에 비례해 스케일)
    void SpawnImpactVFX();

    // 판정 컨텍스트 새로 구성(매 틱 타겟 갱신)
    void RebuildContext();

    // 전달 방식별로 HitActors를 채운다(자기/팀/사망 제외)
    void CollectTargets(FSkillExecutionContext& Ctx) const;

    // 한 후보가 유효 타겟인지(적/사망/래그돌 필터)
    bool IsValidTarget(AActor* Candidate, bool bAvatarIsEnemy) const;

    // 모듈 실행(동시=즉시 전부, 순차=타이머로 하나씩)
    void RunModulesSimultaneous();
    void RunNextSequentialModule();

    UPROPERTY()
    TObjectPtr<USkillDefinition> ActiveSkill = nullptr;

    FVector PendingOrigin = FVector::ZeroVector;
    FVector PendingDirection = FVector::ForwardVector;

    // Detonate에서 채워 모듈 실행에 공유(순차 타이머 사이 GC 추적)
    UPROPERTY()
    FSkillExecutionContext ExecContext;

    int32 SequentialIndex = 0;

    // 지속 필드 경과 시간
    float ChannelElapsed = 0.f;

    FTimerHandle CastTimer;
    FTimerHandle SequentialTimer;
    FTimerHandle FieldTimer;
};
