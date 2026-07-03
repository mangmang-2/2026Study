#include "KnockupEffectModule.h"
#include "GameFramework/Character.h"

void UKnockupEffectModule::Execute(const FSkillExecutionContext& Ctx)
{
    for (const TWeakObjectPtr<AActor>& Weak : Ctx.HitActors)
    {
        ACharacter* Target = Cast<ACharacter>(Weak.Get());
        if (Target == nullptr)
        {
            continue;
        }

        FVector Launch(0.f, 0.f, LaunchUpSpeed);

        // 공중에서 한곳으로 모으고 싶으면 판정 중심 방향 수평 속도 추가
        if (GatherToCenter > 0.f)
        {
            FVector ToCenter = Ctx.Origin - Target->GetActorLocation();
            ToCenter.Z = 0.f;
            Launch += ToCenter.GetSafeNormal() * GatherToCenter;
        }

        Target->LaunchCharacter(Launch, true, true);
    }
}

FText UKnockupEffectModule::GetSummary() const
{
    return FText::FromString(FString::Printf(TEXT("띄우기 %d"), FMath::RoundToInt(LaunchUpSpeed)));
}
