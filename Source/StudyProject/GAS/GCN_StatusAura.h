#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_StatusAura.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

/**
 * 상태이상 지속 VFX용 GameplayCue.
 * GE(화상/출혈/감전/빙결)가 적용·만료될 때 ASC가 자동으로 복제 발동 → 대상 메시에 오라 Niagara on/off.
 * 태그와 Niagara는 BP 자식 큐에서 지정(GameplayCue.Status.X 별로 하나씩).
 */
UCLASS(Blueprintable)
class STUDYPROJECT_API AGCN_StatusAura : public AGameplayCueNotify_Actor
{
    GENERATED_BODY()

public:
    AGCN_StatusAura();

protected:
    virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
    virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
    virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

    // 켤 오라 Niagara (BP 자식에서 지정)
    UPROPERTY(EditDefaultsOnly, Category = "Status")
    TObjectPtr<UNiagaraSystem> AuraVFX;

    // 부착 소켓(없으면 메시 루트)
    UPROPERTY(EditDefaultsOnly, Category = "Status")
    FName AttachSocketName = NAME_None;

private:
    UPROPERTY(Transient)
    TObjectPtr<UNiagaraComponent> ActiveVFX;

    void SpawnAura(AActor* MyTarget);
    void RemoveAura();
};
