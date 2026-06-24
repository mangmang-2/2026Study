#include "PullEffectModule.h"
#include "GameFramework/Character.h"

void UPullEffectModule::Execute(const FSkillExecutionContext& Ctx)
{
    const FVector Anchor = bPullToOrigin ? Ctx.Origin
        : (Ctx.Instigator != nullptr ? Ctx.Instigator->GetActorLocation() : Ctx.Origin);

    for (const TWeakObjectPtr<AActor>& Weak : Ctx.HitActors)
    {
        ACharacter* Target = Cast<ACharacter>(Weak.Get());
        if (Target == nullptr)
        {
            continue;
        }

        FVector Dir = Anchor - Target->GetActorLocation();
        Dir.Z = 0.f;
        Dir = Dir.GetSafeNormal();

        const FVector Launch = Dir * PullStrength + FVector(0.f, 0.f, UpwardBias);
        Target->LaunchCharacter(Launch, true, true);
    }
}

FText UPullEffectModule::GetSummary() const
{
    return FText::FromString(FString::Printf(TEXT("당기기 %d"), FMath::RoundToInt(PullStrength)));
}
