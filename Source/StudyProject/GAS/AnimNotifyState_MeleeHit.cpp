#include "AnimNotifyState_MeleeHit.h"
#include "StudyGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

UAnimNotifyState_MeleeHit::UAnimNotifyState_MeleeHit()
{
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor = false;   // 에디터 미리보기에선 발동 안 함
#endif
}

void UAnimNotifyState_MeleeHit::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
    if (Owner == nullptr)
    {
        return;
    }
    FGameplayEventData Payload;
    Payload.Instigator = Owner;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, StudyTags::Event_Melee_HitStart, Payload);
}

void UAnimNotifyState_MeleeHit::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
    if (Owner == nullptr)
    {
        return;
    }
    FGameplayEventData Payload;
    Payload.Instigator = Owner;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, StudyTags::Event_Melee_HitEnd, Payload);
}
