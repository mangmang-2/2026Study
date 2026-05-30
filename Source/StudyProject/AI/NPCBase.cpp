#include "NPCBase.h"
#include "Character/PlayerCharacter.h"

ANPCBase::ANPCBase()
{
    bHostile = false;
}

void ANPCBase::Interact_Implementation(AActor* Interactor)
{
    UE_LOG(LogTemp, Warning, TEXT("[NPC] Interact — %s"), *GetName());
    Super::Interact_Implementation(Interactor);  // 상점/강화/대화 분기
}
