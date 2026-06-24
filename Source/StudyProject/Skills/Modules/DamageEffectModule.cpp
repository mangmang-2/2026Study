#include "DamageEffectModule.h"
#include "GAS/GE_Damage.h"
#include "GAS/StudyGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

UDamageEffectModule::UDamageEffectModule()
{
    DamageGEClass = UGE_Damage::StaticClass();
}

void UDamageEffectModule::Execute(const FSkillExecutionContext& Ctx)
{
    if (Ctx.InstigatorASC == nullptr || DamageGEClass == nullptr)
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

        FGameplayEffectSpecHandle Spec = Ctx.InstigatorASC->MakeOutgoingSpec(DamageGEClass, 1.f, EffectCtx);
        if (Spec.IsValid() == false)
        {
            continue;
        }

        Spec.Data->SetSetByCallerMagnitude(StudyTags::Data_Damage, DamageAmount);
        Ctx.InstigatorASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
    }
}

FText UDamageEffectModule::GetSummary() const
{
    return FText::FromString(FString::Printf(TEXT("데미지 %d"), FMath::RoundToInt(DamageAmount)));
}
