#include "StunEffectModule.h"
#include "GAS/GE_StatusShocked.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

UStunEffectModule::UStunEffectModule()
{
    StatusGEClass = UGE_StatusShocked::StaticClass();
}

void UStunEffectModule::Execute(const FSkillExecutionContext& Ctx)
{
    if (Ctx.InstigatorASC == nullptr || StatusGEClass == nullptr)
    {
        return;
    }

    for (const TWeakObjectPtr<AActor>& Weak : Ctx.HitActors)
    {
        AActor* Target = Weak.Get();
        if (Target == nullptr)
        {
            continue;
        }

        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
        if (TargetASC == nullptr)
        {
            continue;
        }

        FGameplayEffectContextHandle EffectCtx = Ctx.InstigatorASC->MakeEffectContext();
        EffectCtx.AddSourceObject(Ctx.SourceAbility);

        FGameplayEffectSpecHandle Spec = Ctx.InstigatorASC->MakeOutgoingSpec(StatusGEClass, 1.f, EffectCtx);
        if (Spec.IsValid())
        {
            if (StunDuration > 0.f)
            {
                Spec.Data->SetDuration(StunDuration, true);
            }
            Ctx.InstigatorASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
        }
    }
}

FText UStunEffectModule::GetSummary() const
{
    return FText::FromString(TEXT("스턴(감전)"));
}
