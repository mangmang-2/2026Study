#include "CombatAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "StudyGameplayTags.h"

UCombatAttributeSet::UCombatAttributeSet()
{
    // 기본값(서버에서 BeginPlay/초기화 GE로 덮어씀)
    InitHP(100.f);   InitMaxHP(100.f);
    InitSP(100.f);   InitMaxSP(100.f);
    InitATK(10.f);   InitDEF(5.f);
    InitLevel(1.f);  InitCurrentEXP(0.f); InitRequiredEXP(100.f);
    InitMoveSpeed(500.f);
}

void UCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, HP,          COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, MaxHP,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, SP,          COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, MaxSP,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, ATK,         COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, DEF,         COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, Level,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, CurrentEXP,  COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, RequiredEXP, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, MoveSpeed,   COND_None, REPNOTIFY_Always);
}

void UCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetHPAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHP());
    }
    else if (Attribute == GetSPAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxSP());
    }
}

void UCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    UAbilitySystemComponent* ASC = &Data.Target;
    AActor* TargetActor = Data.Target.AbilityActorInfo.IsValid() ? Data.Target.AbilityActorInfo->AvatarActor.Get() : nullptr;

    // 데미지 meta attribute → HP 차감 후 0으로 리셋
    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        const float LocalDamage = GetDamage();
        SetDamage(0.f);

        if (LocalDamage > 0.f)
        {
            // DEF 경감(간단식: 최소 1 데미지 보장)
            const float Mitigated = FMath::Max(1.f, LocalDamage - GetDEF() * 0.5f);
            SetHP(FMath::Clamp(GetHP() - Mitigated, 0.f, GetMaxHP()));
        }
    }
    else if (Data.EvaluatedData.Attribute == GetHPAttribute())
    {
        SetHP(FMath::Clamp(GetHP(), 0.f, GetMaxHP()));
    }
    else if (Data.EvaluatedData.Attribute == GetSPAttribute())
    {
        SetSP(FMath::Clamp(GetSP(), 0.f, GetMaxSP()));
    }

    const bool bTookDamage = (Data.EvaluatedData.Attribute == GetDamageAttribute());

    // 지속 데미지(Period GE)는 플린치 스팸 방지 위해 제외
    const bool bIsPeriodicDamage = (Data.EffectSpec.Def != nullptr
        && Data.EffectSpec.Def->DurationPolicy != EGameplayEffectDurationType::Instant);

    // 사망 처리 — HP 0 이하 + 아직 Dead 아님
    if (GetHP() <= 0.f && ASC && !ASC->HasMatchingGameplayTag(StudyTags::State_Dead))
    {
        ASC->AddLooseGameplayTag(StudyTags::State_Dead);

        // 사망 이벤트(사망 GA 수신)
        FGameplayEventData Payload;
        Payload.EventTag = StudyTags::Event_Death;
        Payload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
        Payload.Target = TargetActor;
        ASC->HandleGameplayEvent(StudyTags::Event_Death, &Payload);
    }
    else if (bTookDamage && bIsPeriodicDamage == false && GetHP() > 0.f && ASC)
    {
        // 피격 이벤트 전송(HitReact GA가 수신)
        FGameplayEventData Payload;
        Payload.EventTag = StudyTags::Event_HitReact;
        Payload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
        Payload.Target = TargetActor;
        ASC->HandleGameplayEvent(StudyTags::Event_HitReact, &Payload);
    }
}
