#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemData.generated.h"

class UStaticMesh;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EItemType : uint8
{
    Weapon      UMETA(DisplayName = "무기"),
    Armor       UMETA(DisplayName = "방어구"),
    Consumable  UMETA(DisplayName = "소모품"),
    Material    UMETA(DisplayName = "재료"),
    All         UMETA(DisplayName = "전체"),
};

UENUM(BlueprintType)
enum class ESortMode : uint8
{
    ByRarity    UMETA(DisplayName = "희귀도순"),
    ByName      UMETA(DisplayName = "이름순"),
};

UENUM(BlueprintType)
enum class EDamageType : uint8
{
    Normal      UMETA(DisplayName = "일반"),
    Critical    UMETA(DisplayName = "치명타"),
    Heal        UMETA(DisplayName = "회복"),
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Common      UMETA(DisplayName = "일반"),
    Uncommon    UMETA(DisplayName = "고급"),
    Rare        UMETA(DisplayName = "희귀"),
    Epic        UMETA(DisplayName = "영웅"),
    Legendary   UMETA(DisplayName = "전설"),
};

UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
    None     UMETA(DisplayName = "없음"),
    Head     UMETA(DisplayName = "머리"),
    Body     UMETA(DisplayName = "상의"),
    Hands    UMETA(DisplayName = "장갑"),
    Legs     UMETA(DisplayName = "하의"),
    Feet     UMETA(DisplayName = "신발"),
    Shoulder UMETA(DisplayName = "어깨"),
    Arms     UMETA(DisplayName = "팔"),
    Weapon   UMETA(DisplayName = "무기"),
    Shield   UMETA(DisplayName = "방패"),
};

USTRUCT(BlueprintType)
struct STUDYPROJECT_API FItemData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 ItemID = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    FText ItemName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    EItemType ItemType = EItemType::Material;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    EItemRarity Rarity = EItemRarity::Common;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 MaxStack = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 BaseATK = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 BaseDEF = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 BaseHP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    EEquipSlot EquipSlot = EEquipSlot::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 BuyPrice = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 SellPrice = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    float CooldownTime = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 HealAmount = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<USkeletalMesh> ItemMesh;

    // 스태틱 메시 무기용(ItemMesh 대신 사용). 둘 중 하나만 채우면 됨.
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UStaticMesh> ItemStaticMesh;

    // 무기 손소켓 부착 시 적용할 상대 트랜스폼(그립 정렬). 스태틱 무기는 피벗이 중앙이라
    // 손잡이가 손에 오도록 Translation/Rotation을 조정. 기본=항등(중앙 잡음).
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FTransform GripTransform;

    // 강화(레벨>0) 시 무기에 붙는 오라 VFX(나이아가라). 비우면 없음.
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UNiagaraSystem> EnhanceVFX;
};

USTRUCT(BlueprintType)
struct STUDYPROJECT_API FDropTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 MonsterID = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 ItemID = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    float DropRate = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 MinQuantity = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) 
    int32 MaxQuantity = 1;
};

// 드랍 결과 (런타임 전용, DataTable 행 아님)
USTRUCT(BlueprintType)
struct STUDYPROJECT_API FItemDrop
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) 
    int32 ItemID = 0;
    UPROPERTY(BlueprintReadOnly) 
    int32 Quantity = 1;
};
