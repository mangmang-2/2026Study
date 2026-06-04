#include "GA_EnemyAttack.h"
#include "StudyGameplayTags.h"
#include "GE_Damage.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UGA_EnemyAttack::UGA_EnemyAttack()
{
    // AI(서버)에서만 실행
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    bLocksMovement = true;   // 공격 중 이동 정지

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_EnemyAttack);
    SetAssetTags(Tags);

    // 데미지 GE(C++ UGE_Damage). BP 없이 C++로 부여되므로 여기서 지정해야 데미지 적용됨.
    DamageGEClass = UGE_Damage::StaticClass();

    // 콤보 몽타주 4개(Melee Hit 노티파이 포함). 한 발동에서 회복동작을 캔슬하며 크로스블렌드로
    // 1→2→3→4를 이어 매끄러운 콤보. (개별 몽타주를 끝까지 재생+텀 두면 뚝뚝 끊김)
    static ConstructorHelpers::FObjectFinder<UAnimMontage> M1(TEXT("/Game/GAS/Abilities/Montages/AM_Combo_01.AM_Combo_01"));
    static ConstructorHelpers::FObjectFinder<UAnimMontage> M2(TEXT("/Game/GAS/Abilities/Montages/AM_Combo_02.AM_Combo_02"));
    static ConstructorHelpers::FObjectFinder<UAnimMontage> M3(TEXT("/Game/GAS/Abilities/Montages/AM_Combo_03.AM_Combo_03"));
    static ConstructorHelpers::FObjectFinder<UAnimMontage> M4(TEXT("/Game/GAS/Abilities/Montages/AM_Combo_04.AM_Combo_04"));
    if (M1.Succeeded()) { AttackMontage = M1.Object; ComboMontages.Add(M1.Object); }
    if (M2.Succeeded()) { ComboMontages.Add(M2.Object); }
    if (M3.Succeeded()) { ComboMontages.Add(M3.Object); }
    if (M4.Succeeded()) { ComboMontages.Add(M4.Object); }
}

void UGA_EnemyAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 타격 파라미터 + 노티파이 윈도우 리스너(각 몽타주의 Melee Hit 노티파이가 타격 시점 결정).
    // 이벤트 태그 비움 → GE_Damage 적용 시 AttributeSet가 Event.HitReact 전송.
    MeleeDamage = AttackDamage;
    MeleeHitFeel = AttackHitFeel;
    MeleeHitEventTag = FGameplayTag();
    MeleeHitEventMagnitude = 0.f;
    StartMeleeHitWindowListeners();

    ComboStep = 0;
    PlayChainStep();
}

void UGA_EnemyAttack::PlayChainStep()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC == nullptr)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    // 사용할 콤보 배열(없으면 단일 AttackMontage)
    const int32 Count = (ComboMontages.Num() > 0) ? ComboMontages.Num() : 1;
    if (ComboStep >= Count)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    UAnimMontage* M = (ComboMontages.Num() > 0) ? ComboMontages[ComboStep].Get() : AttackMontage.Get();
    if (M == nullptr)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    const float Rate = (AttackPlayRate > 0.f) ? AttackPlayRate : 1.0f;
    // ASC->PlayMontage: 진행 중 몽타주에서 크로스블렌드(회복동작→다음타 매끄럽게) + 멀티 복제
    ASC->PlayMontage(this, CurrentActivationInfo, M, Rate);

    const float PlaySec = (M->GetPlayLength() / Rate);
    ++ComboStep;

    // 마지막 타가 아니면 ChainFraction 지점에 다음 타로 넘어감(회복 캔슬), 마지막이면 끝까지 후 종료
    const bool bLast = (ComboStep >= Count);
    const float NextTime = bLast ? FMath::Max(0.05f, PlaySec * 0.9f) : FMath::Max(0.05f, PlaySec * ChainFraction);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(ChainTimer, this, &UGA_EnemyAttack::PlayChainStep, NextTime, false);
    }
}

void UGA_EnemyAttack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ChainTimer);
    }
    StopMeleeHitWindow();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
