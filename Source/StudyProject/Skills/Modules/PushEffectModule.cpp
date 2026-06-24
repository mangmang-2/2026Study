#include "PushEffectModule.h"
#include "GameFramework/Character.h"

void UPushEffectModule::Execute(const FSkillExecutionContext& Ctx)
{
    const FVector Anchor = bPushFromOrigin ? Ctx.Origin
        : (Ctx.Instigator != nullptr ? Ctx.Instigator->GetActorLocation() : Ctx.Origin);

    for (const TWeakObjectPtr<AActor>& Weak : Ctx.HitActors)
    {
        ACharacter* Target = Cast<ACharacter>(Weak.Get());
        if (Target == nullptr)
        {
            continue;
        }

        FVector Dir = Target->GetActorLocation() - Anchor;
        Dir.Z = 0.f;
        Dir = Dir.GetSafeNormal();

        // 중심과 거의 겹쳐 방향이 0이면 시전자 정면으로 밀기
        if (Dir.IsNearlyZero() && Ctx.Instigator != nullptr)
        {
            Dir = Ctx.Instigator->GetActorForwardVector().GetSafeNormal2D();
        }

        const FVector Launch = Dir * PushStrength + FVector(0.f, 0.f, UpwardBias);
        Target->LaunchCharacter(Launch, true, true);
    }
}

FText UPushEffectModule::GetSummary() const
{
    return FText::FromString(FString::Printf(TEXT("밀기 %d"), FMath::RoundToInt(PushStrength)));
}
