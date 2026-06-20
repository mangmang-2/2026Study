#include "GCN_StatusAura.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

// 큐 태그(GameplayCue.Status.Bleeding)에서 대응 상태이상 태그(Status.Bleeding) 도출
static FGameplayTag StatusTagFromCue(const FGameplayTag& CueTag)
{
    if (CueTag.IsValid() == false)
    {
        return FGameplayTag();
    }
    FString S = CueTag.ToString();
    S.RemoveFromStart(TEXT("GameplayCue."), ESearchCase::CaseSensitive);   // → "Status.Bleeding"
    return FGameplayTag::RequestGameplayTag(FName(*S), /*ErrorIfNotFound*/ false);
}

AGCN_StatusAura::AGCN_StatusAura()
{
    // 큐 제거 시 노티파이 액터 자동 정리. 오라는 대상 메시에 붙여 별도 관리.
    bAutoDestroyOnRemove = true;
}

bool AGCN_StatusAura::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
    SpawnAura(MyTarget);
    return true;
}

bool AGCN_StatusAura::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
    // Join-in-progress 등 OnActive를 못 본 클라에서도 켜지도록
    SpawnAura(MyTarget);
    return true;
}

bool AGCN_StatusAura::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
    // 클라(simulated proxy)에선 주기형(DOT) GE가 매 주기마다 큐 OnRemove를 헛 발생시킨다 →
    // 대상에게 상태이상 태그가 아직 있으면 무시해 VFX 유지.
    // 서버(authority)에선 OnRemove가 진짜 끝에 한 번만 오므로(태그 제거가 살짝 늦어도) 바로 제거해야
    // 한다 — 안 그러면 bAutoDestroyOnRemove로 큐 액터만 사라지고 메시의 Niagara가 고아로 남는다.
    const bool bIsClient = (MyTarget != nullptr && MyTarget->GetWorld() != nullptr
        && MyTarget->GetWorld()->GetNetMode() == NM_Client);
    if (bIsClient)
    {
        UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyTarget);
        const FGameplayTag StatusTag = StatusTagFromCue(Parameters.OriginalTag);
        if (ASC != nullptr && StatusTag.IsValid() && ASC->HasMatchingGameplayTag(StatusTag))
        {
            return true;   // 클라 DOT 주기 churn — 무시
        }
    }
    RemoveAura();
    return true;
}

void AGCN_StatusAura::SpawnAura(AActor* MyTarget)
{
    if (AuraVFX == nullptr || MyTarget == nullptr)
    {
        return;
    }
    if (ActiveVFX != nullptr)
    {
        return;   // 이미 켜져 있음(OnActive/WhileActive 중복 방지)
    }

    USceneComponent* AttachComp = nullptr;
    if (ACharacter* Char = Cast<ACharacter>(MyTarget))
    {
        AttachComp = Char->GetMesh();
    }
    if (AttachComp == nullptr)
    {
        AttachComp = MyTarget->GetRootComponent();
    }
    if (AttachComp == nullptr)
    {
        return;
    }

    ActiveVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
        AuraVFX, AttachComp, AttachSocketName,
        FVector::ZeroVector, FRotator::ZeroRotator,
        EAttachLocation::SnapToTarget, true);
}

void AGCN_StatusAura::RemoveAura()
{
    if (ActiveVFX != nullptr)
    {
        // 잔여 파티클은 자연 소멸(SpawnSystemAttached bAutoDestroy=true라 완료 시 정리)
        ActiveVFX->Deactivate();
        ActiveVFX = nullptr;
    }
}
