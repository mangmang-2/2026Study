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

    DamageGEClass = UGE_Damage::StaticClass();

    // 콤보 몽타주 4개를 한 발동에서 1→2→3→4로 크로스블렌드(회복동작 캔슬)
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

    // 이벤트 태그 비우면 GE_Damage 적용 시 AttributeSet가 Event.HitReact 전송
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
    // PlayMontage: 진행 중 몽타주에서 크로스블렌드 + 멀티 복제
    ASC->PlayMontage(this, CurrentActivationInfo, M, Rate);

    const float PlaySec = (M->GetPlayLength() / Rate);
    ++ComboStep;

    // 마지막 전이면 ChainFraction 지점에 다음 타로, 마지막이면 끝까지 후 종료
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
