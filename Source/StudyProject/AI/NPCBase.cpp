#include "NPCBase.h"
#include "Character/PlayerCharacter.h"

ANPCBase::ANPCBase()
{
    bHostile = false;
}

void ANPCBase::Interact_Implementation(AActor* Interactor)
{
    UE_LOG(LogTemp, Warning, TEXT("[NPC] Interact — %s"), *GetName());
}
