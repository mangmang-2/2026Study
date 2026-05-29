#include "CombatSubsystem.h"
#include "Engine/DataTable.h"

void UCombatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    HitFeedbackTable = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/Data/DT_HitFeedback.DT_HitFeedback"));

    MonsterDataTable = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/Data/DT_MonsterData.DT_MonsterData"));
}

const FHitFeedbackRow* UCombatSubsystem::GetHitFeedback(FName FeedbackID) const
{
    if (!HitFeedbackTable) return nullptr;
    return HitFeedbackTable->FindRow<FHitFeedbackRow>(FeedbackID, TEXT("GetHitFeedback"));
}

const FMonsterDataRow* UCombatSubsystem::GetMonsterData(int32 MonsterID) const
{
    if (!MonsterDataTable) return nullptr;
    FName RowName = *FString::FromInt(MonsterID);
    return MonsterDataTable->FindRow<FMonsterDataRow>(RowName, TEXT("GetMonsterData"));
}
