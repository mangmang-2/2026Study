#pragma once

#include "CoreMinimal.h"
#include "SlotContext.generated.h"

UENUM(BlueprintType)
enum class ESlotContext : uint8
{
    Inventory       UMETA(DisplayName = "인벤토리"),
    Equipment       UMETA(DisplayName = "장비"),
    Shop            UMETA(DisplayName = "상점"),
    Enhance         UMETA(DisplayName = "강화"),
    EnhanceTarget   UMETA(DisplayName = "강화 대상"),
    EnhanceMaterial UMETA(DisplayName = "강화 재료"),
    Trade           UMETA(DisplayName = "거래"),
    TradeRegister   UMETA(DisplayName = "거래 등록"),
};
