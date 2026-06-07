#include "GCN_StatusAura.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

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
