#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ItemData.h"
#include "GameSaveData.generated.h"

USTRUCT(BlueprintType)
struct FItemSaveEntry
{
    GENERATED_BODY()

    UPROPERTY() int32 ItemID = 0;
    UPROPERTY() int32 Quantity = 1;
    UPROPERTY() int32 SlotIndex = -1;
    UPROPERTY() int32 EnhanceLevel = 0;
};

USTRUCT(BlueprintType)
struct FPlayerSaveData
{
    GENERATED_BODY()

    UPROPERTY() FString PlayerName;
    UPROPERTY() int32 Level = 1;
    UPROPERTY() int32 CurrentEXP = 0;
    UPROPERTY() int32 Gold = 0;
    UPROPERTY() FString LastMapName;
    UPROPERTY() FVector LastPosition = FVector::ZeroVector;
    UPROPERTY() FRotator LastRotation = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct FInventorySaveData
{
    GENERATED_BODY()

    UPROPERTY() TArray<FItemSaveEntry> Items;
};

USTRUCT(BlueprintType)
struct FEquipmentSaveData
{
    GENERATED_BODY()

    UPROPERTY() TMap<EEquipSlot, int32> EquippedItems;
};

USTRUCT(BlueprintType)
struct FSettingsSaveData
{
    GENERATED_BODY()

    UPROPERTY() float MasterVolume = 1.f;
    UPROPERTY() float SFXVolume = 1.f;
    UPROPERTY() float BGMVolume = 1.f;
    UPROPERTY() int32 ResolutionIndex = 0;
    UPROPERTY() float MouseSensitivity = 1.f;
};

UCLASS()
class STUDYPROJECT_API UGameSaveData : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() FPlayerSaveData PlayerData;
    UPROPERTY() FInventorySaveData InventoryData;
    UPROPERTY() FEquipmentSaveData EquipmentData;
    UPROPERTY() FSettingsSaveData SettingsData;
};
