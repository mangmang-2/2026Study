#include "SPBarWidget.h"
#include "Components/ProgressBar.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/CombatAttributeSet.h"
#include "GameFramework/Pawn.h"

void USPBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    APawn* OwnerPawn = GetOwningPlayerPawn();
    if (OwnerPawn == nullptr)
    {
        return;
    }

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerPawn);
    if (ASC == nullptr)
    {
        return;
    }

    if (SPBar != nullptr)
    {
        const float SP = ASC->GetNumericAttribute(UCombatAttributeSet::GetSPAttribute());
        const float MaxSP = ASC->GetNumericAttribute(UCombatAttributeSet::GetMaxSPAttribute());
        SPBar->SetPercent(MaxSP > 0.f ? (SP / MaxSP) : 0.f);
    }

    if (HPBar != nullptr)
    {
        const float HP = ASC->GetNumericAttribute(UCombatAttributeSet::GetHPAttribute());
        const float MaxHP = ASC->GetNumericAttribute(UCombatAttributeSet::GetMaxHPAttribute());
        HPBar->SetPercent(MaxHP > 0.f ? (HP / MaxHP) : 0.f);
    }
}
