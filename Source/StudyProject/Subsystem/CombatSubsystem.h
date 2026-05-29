#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/GameData.h"
#include "CombatSubsystem.generated.h"

UCLASS()
class STUDYPROJECT_API UCombatSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // DT_HitFeedback 조회
    const FHitFeedbackRow* GetHitFeedback(FName FeedbackID) const;

    // DT_MonsterData 조회
    const FMonsterDataRow* GetMonsterData(int32 MonsterID) const;

private:
    UPROPERTY()
    TObjectPtr<UDataTable> HitFeedbackTable = nullptr;

    UPROPERTY()
    TObjectPtr<UDataTable> MonsterDataTable = nullptr;
};
