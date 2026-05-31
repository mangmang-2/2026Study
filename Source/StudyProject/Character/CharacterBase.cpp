#include "CharacterBase.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/EnhanceComponent.h"
#include "Data/GameSaveData.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

ACharacterBase::ACharacterBase()
{
    // 인벤토리 컴포넌트
    InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComp"));
    EquipmentComp = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComp"));
    EnhanceComp   = CreateDefaultSubobject<UEnhanceComponent>  (TEXT("EnhanceComp"));

    // Modular Character 파츠 (메인 메시에 Leader Pose)
    HeadMesh     = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMesh"));
    BodyMesh     = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
    HandsMesh    = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandsMesh"));
    LegsMesh     = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh"));
    FeetMesh     = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMesh"));
    ShoulderMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShoulderMesh"));
    ArmsMesh     = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArmsMesh"));
    WeaponMesh   = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    ShieldMesh   = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShieldMesh"));

    SetupModularMesh(HeadMesh);
    SetupModularMesh(BodyMesh);
    SetupModularMesh(HandsMesh);
    SetupModularMesh(LegsMesh);
    SetupModularMesh(FeetMesh);
    SetupModularMesh(ShoulderMesh);
    SetupModularMesh(ArmsMesh);
    SetupModularMesh(WeaponMesh);
    SetupModularMesh(ShieldMesh);
}

void ACharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACharacterBase, Gold);
}

bool ACharacterBase::SpendGold(int32 Amount)
{
    if (!HasAuthority() || Gold < Amount) return false;
    Gold -= Amount;
    return true;
}

void ACharacterBase::AddGold(int32 Amount)
{
    if (!HasAuthority()) return;
    Gold += Amount;
}

void ACharacterBase::OnRep_Gold()
{
    // UI 갱신 필요 시 BP에서 처리 (HUDWidget 골드 텍스트 등)
}

void ACharacterBase::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        LoadCharacter();
    }
}

void ACharacterBase::SetupModularMesh(USkeletalMeshComponent* Part)
{
    Part->SetupAttachment(GetMesh());
    Part->SetCollisionProfileName(TEXT("NoCollision"));
    // Leader Pose: 메인 GetMesh() 본을 따라감 → AnimBP 하나로 전체 동기화
    Part->SetLeaderPoseComponent(GetMesh());
}

void ACharacterBase::SaveCharacter()
{
    UGameSaveData* SaveData = Cast<UGameSaveData>(
        UGameplayStatics::CreateSaveGameObject(UGameSaveData::StaticClass()));
    if (!SaveData) return;

    // 인벤토리 저장
    if (InventoryComp)
    {
        for (int32 i = 0; i < InventoryComp->GetMaxSlots(); ++i)
        {
            const FInventorySlot& Slot = InventoryComp->GetSlot(i);
            if (Slot.IsEmpty()) continue;

            FItemSaveEntry Entry;
            Entry.ItemID       = Slot.ItemID;
            Entry.Quantity     = Slot.Quantity;
            Entry.SlotIndex    = i;
            Entry.EnhanceLevel = Slot.EnhanceLevel;
            SaveData->InventoryData.Items.Add(Entry);
        }
    }

    // 장착 저장
    if (EquipmentComp)
    {
        for (uint8 i = 0; i < (uint8)EEquipSlot::Shield + 1; ++i)
        {
            EEquipSlot Slot = (EEquipSlot)i;
            int32 ItemID = EquipmentComp->GetEquippedItemID(Slot);
            if (ItemID != 0) SaveData->EquipmentData.EquippedItems.Add(Slot, ItemID);
        }
    }

    UGameplayStatics::SaveGameToSlot(SaveData, SaveSlotName, 0);
}

void ACharacterBase::LoadCharacter()
{
    UGameSaveData* SaveData = Cast<UGameSaveData>(
        UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    if (!SaveData) return;

    // 인벤토리 복원
    if (InventoryComp)
    {
        for (const FItemSaveEntry& Entry : SaveData->InventoryData.Items)
        {
            InventoryComp->AddItem(Entry.ItemID, Entry.Quantity);
        }
    }

    // 장착 복원
    if (EquipmentComp)
    {
        for (const auto& Pair : SaveData->EquipmentData.EquippedItems)
        {
            // 인벤에서 찾아서 장착
            if (InventoryComp)
            {
                int32 SlotIdx = InventoryComp->FindItemByID(Pair.Value);
                if (SlotIdx != -1) EquipmentComp->Equip(Pair.Value, SlotIdx);
            }
        }
    }
}
