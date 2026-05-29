#include "AICharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/ItemSubsystem.h"
#include "Interaction/InteractionPromptComponent.h"

AAICharacterBase::AAICharacterBase()
{
    bReplicates = true;

    InteractionPromptComp = CreateDefaultSubobject<UInteractionPromptComponent>(TEXT("InteractionPromptComp"));
    InteractionPromptComp->SetupAttachment(GetRootComponent());
}

void AAICharacterBase::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        CurrentHP = MaxHP;
    }
}

void AAICharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAICharacterBase, CurrentHP);
}

void AAICharacterBase::ReceiveDamage(float Damage, AActor* DamageCauser)
{
    if (!HasAuthority() || !IsAlive()) return;

    CurrentHP = FMath::Max(0.f, CurrentHP - Damage);
    OnDamaged(Damage);

    if (CurrentHP <= 0.f)
    {
        HandleDeath(DamageCauser);
    }
}

void AAICharacterBase::OnRep_CurrentHP()
{
    // HP 변경 시 클라이언트 피드백 — 필요 시 BP에서 HandleDamageEffect 등 구현
}

void AAICharacterBase::HandleDeath(AActor* Killer)
{
    DropItems();
    OnDeath();
}

// ── IInteractable ─────────────────────────────────────────────────────────────

FText AAICharacterBase::GetInteractionPrompt_Implementation() const
{
    if (bHasShop)        return FText::FromString(TEXT("F: 상점"));
    if (DialogueID >= 0) return FText::FromString(TEXT("F: 대화"));
    return FText::FromString(TEXT("F: 상호작용"));
}

bool AAICharacterBase::CanInteract_Implementation(AActor* Interactor) const
{
    return !bHostile && IsAlive();
}

void AAICharacterBase::Interact_Implementation(AActor* Interactor)
{
    UE_LOG(LogTemp, Warning, TEXT("[Interaction] Interact_Implementation called on %s"), *GetName());
    OnInteract(Interactor);
}

// ── 내부 ──────────────────────────────────────────────────────────────────────

void AAICharacterBase::DropItems()
{
    if (MonsterID < 0) return;

    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UItemSubsystem* ItemSub = GI->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return;

    TArray<FItemDrop> Drops = ItemSub->RollDropTable(MonsterID);
    for (const FItemDrop& Drop : Drops)
    {
        // 주변 랜덤 위치에 스폰
        FVector SpawnLoc = GetActorLocation() +
            FVector(FMath::RandRange(-60.f, 60.f), FMath::RandRange(-60.f, 60.f), 50.f);

        ItemSub->SpawnItemInWorld(Drop.ItemID, Drop.Quantity, SpawnLoc, GetWorld());
    }
}
