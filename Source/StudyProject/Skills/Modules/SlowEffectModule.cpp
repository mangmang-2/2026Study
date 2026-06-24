#include "SlowEffectModule.h"
#include "GAS/GE_StatusChilled.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

USlowEffectModule::USlowEffectModule()
{
    StatusGEClass = UGE_StatusChilled::StaticClass();
}

void USlowEffectModule::Execute(const FSkillExecutionContext& Ctx)
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
            if (SlowDuration > 0.f)
            {
                Spec.Data->SetDuration(SlowDuration, true);
            }
            Ctx.InstigatorASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
        }
    }
}

FText USlowEffectModule::GetSummary() const
{
    return FText::FromString(TEXT("둔화"));
}
