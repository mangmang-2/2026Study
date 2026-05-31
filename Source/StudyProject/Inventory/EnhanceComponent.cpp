#include "EnhanceComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Character/CharacterBase.h"
#include "Engine/World.h"

UEnhanceComponent::UEnhanceComponent()
{
    SetIsReplicatedByDefault(true);

    static const FSoftObjectPath EnhancePath(TEXT("/Game/Data/DT_EnhanceRate.DT_EnhanceRate"));
    EnhanceRateTable = Cast<UDataTable>(EnhancePath.TryLoad());
}

void UEnhanceComponent::TryEnhance(int32 InventorySlot)
{
    if (GetOwner()->HasAuthority())
    {
        Internal_TryEnhance(InventorySlot);
        return;
    }
    Server_TryEnhance(InventorySlot);
}

void UEnhanceComponent::Server_TryEnhance_Implementation(int32 InventorySlot)
{
    Internal_TryEnhance(InventorySlot);
}

void UEnhanceComponent::Internal_TryEnhance(int32 InventorySlot)
{
    UInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UInventoryComponent>();
    if (!InvComp) return;

    const FInventorySlot& Slot = InvComp->GetSlot(InventorySlot);
    if (Slot.IsEmpty()) return;
    if (Slot.EnhanceLevel >= MaxEnhanceLevel) return;

    const FEnhanceRateRow* Rate = GetEnhanceRate(Slot.EnhanceLevel);
    if (!Rate) return;

    // 재료 보유 확인 (소모 전에 먼저 확인)
    if (!InvComp->HasItem(Rate->MaterialID, Rate->MaterialCount)) return;

    // 골드 확인 및 차감 (소유 캐릭터의 Gold). 부족하면 강화 중단(재료도 소모 안 함)
    ACharacterBase* OwnerChar = Cast<ACharacterBase>(GetOwner());
    if (Rate->GoldCost > 0)
    {
        if (OwnerChar == nullptr || !OwnerChar->SpendGold(Rate->GoldCost))
        {
            return;
        }
    }

    // 재료 소모
    int32 MatSlot = InvComp->FindItemByID(Rate->MaterialID);
    if (MatSlot != -1) InvComp->RemoveItem(MatSlot, Rate->MaterialCount);

    // 성공 판정
    const bool bSuccess = FMath::FRand() <= Rate->SuccessRate;
    const int32 NewLevel = bSuccess
        ? Slot.EnhanceLevel + 1
        : FMath::Max(0, Slot.EnhanceLevel + Rate->FailPenalty);

    // 실제 인벤토리 슬롯에 강화 레벨 반영(서버 권위)
    InvComp->SetSlotEnhanceLevel(InventorySlot, NewLevel);

    Multicast_OnEnhanceResult(bSuccess, NewLevel);
}

void UEnhanceComponent::Multicast_OnEnhanceResult_Implementation(bool bSuccess, int32 NewLevel)
{
    OnEnhanceResult.Broadcast(bSuccess, NewLevel);
    OnEnhanceVisualEffect(bSuccess);
}

const FEnhanceRateRow* UEnhanceComponent::GetEnhanceRate(int32 Level) const
{
    if (!EnhanceRateTable) return nullptr;
    return EnhanceRateTable->FindRow<FEnhanceRateRow>(FName(*FString::FromInt(Level)), TEXT("GetEnhanceRate"));
}
