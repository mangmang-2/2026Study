#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_MeleeHit.generated.h"

/**
 * 근접 타격 윈도우 노티파이.
 * 몽타주에서 무기를 휘두르는 구간(칼이 실제로 닿는 프레임)에 깔아두면:
 *  - 구간 시작 시 소유자 ASC로 Event.Melee.HitStart 전송 → 공격 GA가 타격 판정 윈도우를 연다
 *  - 구간 끝/중단 시 Event.Melee.HitEnd 전송 → 윈도우를 닫는다
 * 윈도우 동안 GA가 매 프레임 트레이스하므로, 타이밍이 스윙마다 달라도 정확히 맞는다.
 */
UCLASS(meta = (DisplayName = "Melee Hit Window"))
class STUDYPROJECT_API UAnimNotifyState_MeleeHit : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UAnimNotifyState_MeleeHit();

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
    virtual FString GetNotifyName_Implementation() const override { return TEXT("Melee Hit"); }
#endif
};
