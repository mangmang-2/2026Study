#pragma once

#include "CoreMinimal.h"
#include "AI/AICharacterBase.h"
#include "NPCBase.generated.h"

UCLASS()
class STUDYPROJECT_API ANPCBase : public AAICharacterBase
{
    GENERATED_BODY()

public:
    ANPCBase();

    virtual void Interact_Implementation(AActor* Interactor) override;
};
