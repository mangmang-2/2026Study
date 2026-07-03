#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemySpawnerWidget.generated.h"

class UButton;
class APlayerCharacter;

/**
 * 디버그용 적/보스 스폰 위젯(코드 전용 — WBP 불필요).
 * 화면 우상단에 [적 소환]/[보스 소환]/[전체 제거] 버튼. 평상시엔 적 없이 테스트하고 필요할 때 소환.
 */
UCLASS()
class STUDYPROJECT_API UEnemySpawnerWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UFUNCTION() void OnSpawnEnemy();
    UFUNCTION() void OnSpawnBoss();
    UFUNCTION() void OnSpawnDummy();
    UFUNCTION() void OnClear();

    APlayerCharacter* GetPlayerChar() const;

    UPROPERTY() TObjectPtr<UButton> SpawnEnemyBtn = nullptr;
    UPROPERTY() TObjectPtr<UButton> SpawnBossBtn  = nullptr;
    UPROPERTY() TObjectPtr<UButton> SpawnDummyBtn = nullptr;
    UPROPERTY() TObjectPtr<UButton> ClearBtn      = nullptr;
};
