#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/InteractableInterface.h"
#include "AICharacterBase.generated.h"

class UInteractionPromptComponent;

UCLASS(Abstract)
class STUDYPROJECT_API AAICharacterBase : public ACharacter, public IInteractable
{
    GENERATED_BODY()

public:
    AAICharacterBase();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<UInteractionPromptComponent> InteractionPromptComp;

    // 피격 — 1단계 단순 버전 (2단계에서 GAS ApplyGameplayEffect로 교체)
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void ReceiveDamage(float Damage, AActor* DamageCauser);

    UFUNCTION(BlueprintCallable, Category = "AI")
    bool IsAlive() const { return CurrentHP > 0.f; }

    UFUNCTION(BlueprintPure, Category = "AI")
    bool IsHostile() const { return bHostile; }

    UFUNCTION(BlueprintPure, Category = "AI")
    bool HasShop() const { return bHasShop; }

    UFUNCTION(BlueprintPure, Category = "AI")
    bool HasEnhance() const { return bHasEnhance; }

    UFUNCTION(BlueprintPure, Category = "AI")
    int32 GetShopID() const { return ShopID; }

    UFUNCTION(BlueprintPure, Category = "AI")
    int32 GetDialogueID() const { return DialogueID; }

    // IInteractable — BlueprintNativeEvent _Implementation
    virtual FText GetInteractionPrompt_Implementation() const override;
    virtual bool  CanInteract_Implementation(AActor* Interactor) const override;
    virtual void  Interact_Implementation(AActor* Interactor) override;

protected:
    // ── 역할 설정 (DataTable 기반 → 여기선 BP에서 설정) ─────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    bool bHostile = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    int32 MonsterID = -1;           // DT_MonsterData / DT_DropTable 키

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    bool bHasShop = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    bool bHasEnhance = false;       // 강화 NPC 여부 (상점과 분기)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    int32 ShopID = -1;              // DT_ShopInventory 키

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    int32 DialogueID = -1;          // DT_DialogueData 키

    // ── 전투 스탯 ────────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float MaxHP = 100.f;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    float CurrentHP = 0.f;

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintImplementableEvent, Category = "AI")
    void OnDeath();

    UFUNCTION(BlueprintImplementableEvent, Category = "AI")
    void OnDamaged(float Damage);

    // 상호작용 시작 — BP에서 대화/상점 UI 열기
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteract(AActor* Interactor);

private:
    UFUNCTION()
    void OnRep_CurrentHP();

    void HandleDeath(AActor* Killer);
    void DropItems();               // ItemSubsystem->RollDropTable → SpawnItemInWorld
};
