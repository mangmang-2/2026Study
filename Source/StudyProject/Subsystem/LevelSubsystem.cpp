#include "LevelSubsystem.h"
#include "Engine/DataTable.h"
#include "Math/UnrealMathUtility.h"

void ULevelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LevelUpTable = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/Data/DT_LevelUp.DT_LevelUp"));

    EnhanceRateTable = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/Data/DT_EnhanceRate.DT_EnhanceRate"));
}

const FLevelUpRow* ULevelSubsystem::GetLevelUpData(int32 Level) const
{
    if (!LevelUpTable) return nullptr;
    FName RowName = *FString::FromInt(Level);
    return LevelUpTable->FindRow<FLevelUpRow>(RowName, TEXT("GetLevelUpData"));
}

bool ULevelSubsystem::CanLevelUp(int32 CurrentLevel, int32 CurrentEXP) const
{
    const FLevelUpRow* Row = GetLevelUpData(CurrentLevel);
    return Row && CurrentEXP >= Row->RequiredEXP;
}

const FEnhanceRateRow* ULevelSubsystem::GetEnhanceRate(int32 CurrentLevel) const
{
    if (!EnhanceRateTable) return nullptr;
    FName RowName = *FString::FromInt(CurrentLevel);
    return EnhanceRateTable->FindRow<FEnhanceRateRow>(RowName, TEXT("GetEnhanceRate"));
}

bool ULevelSubsystem::RollEnhance(int32 CurrentLevel) const
{
    const FEnhanceRateRow* Row = GetEnhanceRate(CurrentLevel);
    if (!Row) return false;
    return FMath::FRand() < Row->SuccessRate;
}
