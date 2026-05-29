#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameData.generated.h"

// ── DT_EnhanceRate ────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct STUDYPROJECT_API FEnhanceRateRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  CurrentLevel  = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float  SuccessRate   = 1.f;  // 0.0~1.0
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  GoldCost      = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  MaterialID    = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  MaterialCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  FailPenalty   = 0;    // 0=유지, -1=하락
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  ATKPerLevel   = 5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  DEFPerLevel   = 3;
};

// ── DT_ShopInventory ─────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct STUDYPROJECT_API FShopInventoryRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ShopID     = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ItemID     = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StockLimit = -1;   // -1 = 무한
};

// ── DT_LevelUp ───────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct STUDYPROJECT_API FLevelUpRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Level       = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RequiredEXP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BonusHP     = 10;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BonusSP     = 5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BonusATK    = 2;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BonusDEF    = 1;
};

// ── DT_MonsterData ───────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct STUDYPROJECT_API FMonsterDataRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  MonsterID  = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText  MonsterName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float  MaxHP      = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float  ATK        = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float  DEF        = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  EXPReward  = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  GoldReward = 10;
};

// ── DT_HitFeedback ───────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct STUDYPROJECT_API FHitFeedbackRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName  FeedbackID        = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float  HitStopDuration   = 0.05f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float  ShakeIntensity    = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float  KnockbackForce    = 300.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float  ParticleScale     = 1.f;
};

// ── DT_DialogueData ──────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EDialogueAction : uint8
{
    NextNode     UMETA(DisplayName = "다음 대사"),
    OpenShop     UMETA(DisplayName = "상점 열기"),
    AcceptQuest  UMETA(DisplayName = "퀘스트 수락"),
    CompleteQuest UMETA(DisplayName = "퀘스트 완료"),
    Exit         UMETA(DisplayName = "대화 종료"),
};

USTRUCT(BlueprintType)
struct STUDYPROJECT_API FDialogueChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText            ChoiceText;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EDialogueAction  Action     = EDialogueAction::NextNode;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32            NextNode   = 0;   // NextNode 액션 시 이동할 NodeIndex
};

USTRUCT(BlueprintType)
struct STUDYPROJECT_API FDialogueRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32                   DialogueID   = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32                   NodeIndex    = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText                   SpeakerName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText                   DialogueText;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool                    bHasChoices  = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDialogueChoice> Choices;
};
