#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/GameData.h"
#include "LevelSubsystem.generated.h"

UCLASS()
class STUDYPROJECT_API ULevelSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // DT_LevelUp 조회
    const FLevelUpRow* GetLevelUpData(int32 Level) const;

    // 레벨업 가능 여부 확인
    UFUNCTION(BlueprintCallable, Category = "Level")
    bool CanLevelUp(int32 CurrentLevel, int32 CurrentEXP) const;

    // DT_EnhanceRate 조회
    const FEnhanceRateRow* GetEnhanceRate(int32 CurrentLevel) const;

    // 강화 성공 판정
    UFUNCTION(BlueprintCallable, Category = "Enhance")
    bool RollEnhance(int32 CurrentLevel) const;

private:
    UPROPERTY()
    TObjectPtr<UDataTable> LevelUpTable = nullptr;

    UPROPERTY()
    TObjectPtr<UDataTable> EnhanceRateTable = nullptr;
};
