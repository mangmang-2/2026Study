#include "EquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Character/CharacterBase.h"
#include "Inventory/InventoryComponent.h"
#include "Subsystem/ItemSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

namespace
{
    // SK_Skeleton_base에 저작된 손잡이 소켓
    const FName WeaponSocketName(TEXT("HandSocket_R"));
    const FName ShieldSocketName(TEXT("HandSocket_L"));

    const EEquipSlot AllEquipSlots[] = {
        EEquipSlot::Head, EEquipSlot::Body, EEquipSlot::Hands, EEquipSlot::Legs,
        EEquipSlot::Feet, EEquipSlot::Shoulder, EEquipSlot::Arms,
        EEquipSlot::Weapon, EEquipSlot::Shield };
}

// ── FEquippedItemsData ────────────────────────────────────────────────

int32 FEquippedItemsData::Get(EEquipSlot Slot) const
{
    switch (Slot)
    {
    case EEquipSlot::Head:     return HeadItemID;
    case EEquipSlot::Body:     return BodyItemID;
    case EEquipSlot::Hands:    return HandsItemID;
    case EEquipSlot::Legs:     return LegsItemID;
    case EEquipSlot::Feet:     return FeetItemID;
    case EEquipSlot::Shoulder: return ShoulderItemID;
    case EEquipSlot::Arms:     return ArmsItemID;
    case EEquipSlot::Weapon:   return WeaponItemID;
    case EEquipSlot::Shield:   return ShieldItemID;
    default: return 0;
    }
}

void FEquippedItemsData::Set(EEquipSlot Slot, int32 ItemID)
{
    switch (Slot)
    {
    case EEquipSlot::Head:     HeadItemID     = ItemID; break;
    case EEquipSlot::Body:     BodyItemID     = ItemID; break;
    case EEquipSlot::Hands:    HandsItemID    = ItemID; break;
    case EEquipSlot::Legs:     LegsItemID     = ItemID; break;
    case EEquipSlot::Feet:     FeetItemID     = ItemID; break;
    case EEquipSlot::Shoulder: ShoulderItemID = ItemID; break;
    case EEquipSlot::Arms:     ArmsItemID     = ItemID; break;
    case EEquipSlot::Weapon:   WeaponItemID   = ItemID; break;
    case EEquipSlot::Shield:   ShieldItemID   = ItemID; break;
    default: break;
    }
}

void FEquippedItemsData::Clear(EEquipSlot Slot)
{
    Set(Slot, 0);
    SetEnhance(Slot, 0);
}

int32 FEquippedItemsData::GetEnhance(EEquipSlot Slot) const
{
    const int32 Idx = (int32)Slot;
    return EnhanceLevels.IsValidIndex(Idx) ? EnhanceLevels[Idx] : 0;
}

void FEquippedItemsData::SetEnhance(EEquipSlot Slot, int32 Level)
{
    const int32 Idx = (int32)Slot;
    if (EnhanceLevels.Num() <= Idx)
    {
        EnhanceLevels.SetNumZeroed(Idx + 1);
    }
    EnhanceLevels[Idx] = Level;
}

TArray<TPair<EEquipSlot, int32>> FEquippedItemsData::GetAll() const
{
    TArray<TPair<EEquipSlot, int32>> Result;
    if (HeadItemID     != 0) Result.Add({ EEquipSlot::Head,     HeadItemID });
    if (BodyItemID     != 0) Result.Add({ EEquipSlot::Body,     BodyItemID });
    if (HandsItemID    != 0) Result.Add({ EEquipSlot::Hands,    HandsItemID });
    if (LegsItemID     != 0) Result.Add({ EEquipSlot::Legs,     LegsItemID });
    if (FeetItemID     != 0) Result.Add({ EEquipSlot::Feet,     FeetItemID });
    if (ShoulderItemID != 0) Result.Add({ EEquipSlot::Shoulder, ShoulderItemID });
    if (ArmsItemID     != 0) Result.Add({ EEquipSlot::Arms,     ArmsItemID });
    if (WeaponItemID   != 0) Result.Add({ EEquipSlot::Weapon,   WeaponItemID });
    if (ShieldItemID   != 0) Result.Add({ EEquipSlot::Shield,   ShieldItemID });
    return Result;
}

// ── UEquipmentComponent ───────────────────────────────────────────────

UEquipmentComponent::UEquipmentComponent()
{
    SetIsReplicatedByDefault(true);
    Loadouts.SetNum(NumLoadouts);
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UEquipmentComponent, EquippedItems);
    DOREPLIFETIME(UEquipmentComponent, WeaponEnhanceLevel);
    DOREPLIFETIME(UEquipmentComponent, Loadouts);
    DOREPLIFETIME(UEquipmentComponent, ActiveLoadout);
}

bool UEquipmentComponent::Equip(int32 ItemID, int32 FromInventorySlot)
{
    if (GetOwner()->HasAuthority())
    {
        UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
        const FItemData* Data = ItemSub ? ItemSub->GetItemData(ItemID) : nullptr;
        if (!Data || Data->EquipSlot == EEquipSlot::None) return false;

        UInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UInventoryComponent>();
        const EEquipSlot Slot = Data->EquipSlot;

        // 장착할 아이템의 강화 레벨 캡처(인벤에서 제거되기 전)
        const int32 IncomingEnhance = InvComp ? InvComp->GetSlot(FromInventorySlot).EnhanceLevel : 0;

        // 기존 장착 아이템 → 강화 레벨 유지한 채 인벤으로 반환
        const int32 OldID = EquippedItems.Get(Slot);
        if (OldID != 0 && InvComp)
        {
            InvComp->AddItemWithEnhance(OldID, EquippedItems.GetEnhance(Slot));
        }

        // 인벤에서 제거 후 장착
        if (InvComp) InvComp->RemoveItem(FromInventorySlot, 1);

        EquippedItems.Set(Slot, ItemID);
        EquippedItems.SetEnhance(Slot, IncomingEnhance);
        if (Slot == EEquipSlot::Weapon)
        {
            WeaponEnhanceLevel = IncomingEnhance;
        }
        ApplyMeshForSlot(Slot, ItemID);
        OnEquipmentChanged.Broadcast();
        OnEquipmentMutated();   // 장착 → 활성 세트 반영 + 디스크 저장
        return true;
    }
    Server_Equip(ItemID, FromInventorySlot);
    return true;
}

bool UEquipmentComponent::Unequip(EEquipSlot Slot)
{
    if (GetOwner()->HasAuthority())
    {
        int32 EquippedID = EquippedItems.Get(Slot);
        if (EquippedID == 0) return false;

        UInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UInventoryComponent>();
        if (InvComp && !InvComp->IsFull())
        {
            // 강화 레벨 유지한 채 인벤으로 반환
            InvComp->AddItemWithEnhance(EquippedID, EquippedItems.GetEnhance(Slot));
        }

        if (Slot == EEquipSlot::Weapon)
        {
            WeaponEnhanceLevel = 0;
        }
        ClearMeshForSlot(Slot);
        EquippedItems.Clear(Slot);
        OnEquipmentChanged.Broadcast();
        OnEquipmentMutated();   // 해제 → 활성 세트 반영 + 디스크 저장
        return true;
    }
    Server_Unequip(Slot);
    return true;
}

int32 UEquipmentComponent::GetEquippedItemID(EEquipSlot Slot) const
{
    return EquippedItems.Get(Slot);
}

bool UEquipmentComponent::IsSlotOccupied(EEquipSlot Slot) const
{
    return EquippedItems.IsOccupied(Slot);
}

int32 UEquipmentComponent::GetTotalBonusATK() const
{
    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return 0;

    int32 Total = 0;
    for (const auto& Pair : EquippedItems.GetAll())
    {
        const FItemData* Data = ItemSub->GetItemData(Pair.Value);
        if (Data) Total += Data->BaseATK;
    }
    return Total;
}

int32 UEquipmentComponent::GetTotalBonusDEF() const
{
    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return 0;

    int32 Total = 0;
    for (const auto& Pair : EquippedItems.GetAll())
    {
        const FItemData* Data = ItemSub->GetItemData(Pair.Value);
        if (Data) Total += Data->BaseDEF;
    }
    return Total;
}

int32 UEquipmentComponent::GetTotalBonusHP() const
{
    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return 0;

    int32 Total = 0;
    for (const auto& Pair : EquippedItems.GetAll())
    {
        const FItemData* Data = ItemSub->GetItemData(Pair.Value);
        if (Data) Total += Data->BaseHP;
    }
    return Total;
}

void UEquipmentComponent::OnRep_EquippedItems()
{
    RefreshAllMeshes();   // 빈 슬롯도 정리(세트 전환 시 벗겨진 슬롯 메시 제거)
    OnEquipmentChanged.Broadcast();
}

void UEquipmentComponent::Server_Equip_Implementation(int32 ItemID, int32 FromInventorySlot)
{
    Equip(ItemID, FromInventorySlot);
}

void UEquipmentComponent::Server_Unequip_Implementation(EEquipSlot Slot)
{
    Unequip(Slot);
}

void UEquipmentComponent::EnhanceEquippedWeapon(int32 Delta)
{
    if (GetOwner()->HasAuthority())
    {
        if (EquippedItems.Get(EEquipSlot::Weapon) == 0)
        {
            return;   // 무기 없음
        }
        WeaponEnhanceLevel = FMath::Clamp(WeaponEnhanceLevel + Delta, 0, 10);
        EquippedItems.SetEnhance(EEquipSlot::Weapon, WeaponEnhanceLevel);   // 해제 시 보존되게
        UpdateWeaponAura();   // 서버(호스트) 본인 갱신 — 클라는 OnRep로
        return;
    }
    Server_EnhanceEquippedWeapon(Delta);
}

void UEquipmentComponent::Server_EnhanceEquippedWeapon_Implementation(int32 Delta)
{
    EnhanceEquippedWeapon(Delta);
}

bool UEquipmentComponent::IsLoadoutEmpty(int32 Index) const
{
    if (Loadouts.IsValidIndex(Index) == false)
    {
        return true;
    }
    return Loadouts[Index].GetAll().Num() == 0;
}

void UEquipmentComponent::SetActiveLoadout(int32 Index)
{
    ActiveLoadout = Index;
}

FEquippedItemsData UEquipmentComponent::GetLoadout(int32 Index) const
{
    return Loadouts.IsValidIndex(Index) ? Loadouts[Index] : FEquippedItemsData();
}

void UEquipmentComponent::LoadLoadout(int32 Index, const FEquippedItemsData& Data)
{
    if (GetOwner()->HasAuthority() && Loadouts.IsValidIndex(Index))
    {
        Loadouts[Index] = Data;
    }
}

void UEquipmentComponent::SaveLoadout(int32 Index)
{
    if (GetOwner()->HasAuthority() == false)
    {
        Server_SaveLoadout(Index);
        return;
    }
    if (Loadouts.IsValidIndex(Index))
    {
        Loadouts[Index] = EquippedItems;
        OnEquipmentChanged.Broadcast();
    }
}

void UEquipmentComponent::Server_SaveLoadout_Implementation(int32 Index)
{
    SaveLoadout(Index);
}

void UEquipmentComponent::ApplyLoadout(int32 Index)
{
    if (Loadouts.IsValidIndex(Index) == false)
    {
        return;
    }
    ActiveLoadout = Index;   // 활성 세트(하이라이트·장착 대상)

    if (GetOwner()->HasAuthority() == false)
    {
        Server_ApplyLoadout(Index);
        return;
    }

    // 세트 전환 = 가방 안 건드리고 그 세트 장비로 교체. 이전 세트 장비는 그 세트에 그대로 보관됨.
    EquippedItems = Loadouts[Index];
    WeaponEnhanceLevel = EquippedItems.GetEnhance(EEquipSlot::Weapon);
    RefreshAllMeshes();
    UpdateWeaponAura();
    OnEquipmentChanged.Broadcast();
    PersistToDisk();
}

void UEquipmentComponent::Server_ApplyLoadout_Implementation(int32 Index)
{
    ApplyLoadout(Index);
}

void UEquipmentComponent::RefreshAllMeshes()
{
    for (EEquipSlot Slot : AllEquipSlots)
    {
        const int32 Id = EquippedItems.Get(Slot);
        if (Id != 0)
        {
            ApplyMeshForSlot(Slot, Id);
        }
        else
        {
            ClearMeshForSlot(Slot);
        }
    }
}

void UEquipmentComponent::InitFromActiveLoadout()
{
    // 로드 직후: 활성 세트를 현재 장비로 반영(디스크 저장은 안 함)
    if (Loadouts.IsValidIndex(ActiveLoadout))
    {
        EquippedItems = Loadouts[ActiveLoadout];
    }
    WeaponEnhanceLevel = EquippedItems.GetEnhance(EEquipSlot::Weapon);
    RefreshAllMeshes();
    UpdateWeaponAura();
    OnEquipmentChanged.Broadcast();
}

void UEquipmentComponent::OnEquipmentMutated()
{
    // 장착/해제 → 현재 장비를 활성 세트에 반영 + 디스크 저장
    if (Loadouts.IsValidIndex(ActiveLoadout))
    {
        SaveLoadout(ActiveLoadout);
    }
    PersistToDisk();
}

void UEquipmentComponent::PersistToDisk()
{
    if (ACharacterBase* Char = Cast<ACharacterBase>(GetOwner()))
    {
        if (Char->HasAuthority())
        {
            Char->SaveCharacter();
        }
    }
}

void UEquipmentComponent::ApplyMeshForSlot(EEquipSlot Slot, int32 ItemID)
{
    ACharacterBase* Char = Cast<ACharacterBase>(GetOwner());
    if (Char == nullptr)
    {
        return;
    }

    USkeletalMeshComponent* Target = nullptr;
    switch (Slot)
    {
    case EEquipSlot::Head:     Target = Char->HeadMesh;     break;
    case EEquipSlot::Body:     Target = Char->BodyMesh;     break;
    case EEquipSlot::Hands:    Target = Char->HandsMesh;    break;
    case EEquipSlot::Legs:     Target = Char->LegsMesh;     break;
    case EEquipSlot::Feet:     Target = Char->FeetMesh;     break;
    case EEquipSlot::Shoulder: Target = Char->ShoulderMesh; break;
    case EEquipSlot::Arms:     Target = Char->ArmsMesh;     break;
    case EEquipSlot::Weapon:   Target = Char->WeaponMesh;   break;
    case EEquipSlot::Shield:   Target = Char->ShieldMesh;   break;
    default:                   break;
    }
    if (Target == nullptr)
    {
        return;
    }

    UItemSubsystem* ItemSub = GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>();
    const FItemData* Data = ItemSub ? ItemSub->GetItemData(ItemID) : nullptr;
    if (Data == nullptr)
    {
        return;
    }

    // 무기 슬롯에 스태틱 메시가 지정돼 있으면 스태틱 메시 컴포넌트로 장착
    if (Slot == EEquipSlot::Weapon && !Data->ItemStaticMesh.IsNull())
    {
        // 스켈레탈 무기 메시는 비우고, 스태틱 메시를 손 소켓에 부착
        Target->SetSkeletalMeshAsset(nullptr);
        if (Char->WeaponStaticMesh != nullptr)
        {
            Char->WeaponStaticMesh->SetStaticMesh(Data->ItemStaticMesh.LoadSynchronous());
            Char->WeaponStaticMesh->AttachToComponent(Char->GetMesh(),
                FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
            // 손잡이가 손에 오도록 상대 트랜스폼(스태틱은 피벗 중앙)
            Char->WeaponStaticMesh->SetRelativeTransform(Data->GripTransform);
            Char->WeaponStaticMesh->SetVisibility(true);
        }
        UpdateWeaponAura();
        return;
    }

    USkeletalMesh* Mesh = Data->ItemMesh.LoadSynchronous();
    Target->SetSkeletalMeshAsset(Mesh);

    if (Slot == EEquipSlot::Weapon || Slot == EEquipSlot::Shield)
    {
        // 무기/방패: 자체 스켈레톤이라 Leader Pose 불가 → 손 본 소켓에 부착
        const FName SocketName = (Slot == EEquipSlot::Weapon) ? WeaponSocketName : ShieldSocketName;
        Target->SetLeaderPoseComponent(nullptr);
        Target->AttachToComponent(Char->GetMesh(),
            FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);

        // 스켈레탈 무기로 교체됐으면 스태틱 무기 메시는 숨김
        if (Slot == EEquipSlot::Weapon && Char->WeaponStaticMesh != nullptr)
        {
            Char->WeaponStaticMesh->SetStaticMesh(nullptr);
        }
    }
    else
    {
        // 머리/상의 방어구: 베이스 바디 스켈레톤을 따라가는 Leader Pose
        Target->SetLeaderPoseComponent(Char->GetMesh());
    }

    if (Slot == EEquipSlot::Weapon)
    {
        UpdateWeaponAura();
    }
}

void UEquipmentComponent::UpdateWeaponAura()
{
    ACharacterBase* Char = Cast<ACharacterBase>(GetOwner());
    if (Char == nullptr || Char->WeaponAuraVFX == nullptr)
    {
        return;
    }

    const int32 WeaponID = EquippedItems.Get(EEquipSlot::Weapon);
    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;
    const FItemData* Data = (ItemSub && WeaponID != 0) ? ItemSub->GetItemData(WeaponID) : nullptr;

    // 강화 레벨>0 + EnhanceVFX 있으면 부착 재생, 아니면 끔
    UNiagaraSystem* VFX = (Data && WeaponEnhanceLevel > 0) ? Data->EnhanceVFX.LoadSynchronous() : nullptr;
    if (VFX != nullptr)
    {
        // 스태틱 무기면 스태틱 메시에, 아니면 스켈레탈 무기 메시에 부착
        USceneComponent* AttachTo = (Data && !Data->ItemStaticMesh.IsNull())
            ? Cast<USceneComponent>(Char->WeaponStaticMesh)
            : Cast<USceneComponent>(Char->WeaponMesh);
        if (AttachTo != nullptr)
        {
            Char->WeaponAuraVFX->AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetIncludingScale);
        }
        Char->WeaponAuraVFX->SetAsset(VFX);
        Char->WeaponAuraVFX->Activate(true);
    }
    else
    {
        Char->WeaponAuraVFX->Deactivate();
        Char->WeaponAuraVFX->SetAsset(nullptr);
    }
}

void UEquipmentComponent::ClearMeshForSlot(EEquipSlot Slot)
{
    ACharacterBase* Char = Cast<ACharacterBase>(GetOwner());
    if (Char == nullptr)
    {
        return;
    }

    USkeletalMeshComponent* Target = nullptr;
    switch (Slot)
    {
    case EEquipSlot::Head:     Target = Char->HeadMesh;     break;
    case EEquipSlot::Body:     Target = Char->BodyMesh;     break;
    case EEquipSlot::Hands:    Target = Char->HandsMesh;    break;
    case EEquipSlot::Legs:     Target = Char->LegsMesh;     break;
    case EEquipSlot::Feet:     Target = Char->FeetMesh;     break;
    case EEquipSlot::Shoulder: Target = Char->ShoulderMesh; break;
    case EEquipSlot::Arms:     Target = Char->ArmsMesh;     break;
    case EEquipSlot::Weapon:   Target = Char->WeaponMesh;   break;
    case EEquipSlot::Shield:   Target = Char->ShieldMesh;   break;
    default:                   break;
    }
    if (Target == nullptr)
    {
        return;
    }

    Target->SetSkeletalMeshAsset(nullptr);

    // 무기 해제: 스태틱 메시·오라도 정리
    if (Slot == EEquipSlot::Weapon)
    {
        if (Char->WeaponStaticMesh != nullptr)
        {
            Char->WeaponStaticMesh->SetStaticMesh(nullptr);
        }
        WeaponEnhanceLevel = 0;
        if (Char->WeaponAuraVFX != nullptr)
        {
            Char->WeaponAuraVFX->Deactivate();
            Char->WeaponAuraVFX->SetAsset(nullptr);
        }
    }
}

void UEquipmentComponent::OnRep_WeaponEnhanceLevel()
{
    UpdateWeaponAura();
}