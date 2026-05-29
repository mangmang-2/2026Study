#include "EnhanceComponent.h"
#include "Inventory/InventoryComponent.h"
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

    // 재료 보유 확인
    if (!InvComp->HasItem(Rate->MaterialID, Rate->MaterialCount)) return;

    // TODO: 골드 차감 확인 (PlayerStatComponent 연동 후 구현)

    // 재료 소모
    int32 MatSlot = InvComp->FindItemByID(Rate->MaterialID);
    if (MatSlot != -1) InvComp->RemoveItem(MatSlot, Rate->MaterialCount);

    // 성공 판정
    bool bSuccess = FMath::FRand() <= Rate->SuccessRate;

    if (bSuccess)
    {
        Multicast_OnEnhanceResult(true, Slot.EnhanceLevel + 1);
    }
    else
    {
        int32 NewLevel = FMath::Max(0, Slot.EnhanceLevel + Rate->FailPenalty);
        Multicast_OnEnhanceResult(false, NewLevel);
    }
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
