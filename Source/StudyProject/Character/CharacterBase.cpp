#include "CharacterBase.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/EnhanceComponent.h"
#include "Inventory/ShopComponent.h"
#include "GAS/CombatAbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "GAS/StudyGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Data/GameSaveData.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/DamageNumberWidget.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

ACharacterBase::ACharacterBase()
{
    // 인벤토리 컴포넌트
    InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComp"));
    EquipmentComp = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComp"));
    EnhanceComp   = CreateDefaultSubobject<UEnhanceComponent>  (TEXT("EnhanceComp"));
    ShopComp      = CreateDefaultSubobject<UShopComponent>     (TEXT("ShopComp"));

    // GAS — ASC + AttributeSet (AttributeSet은 소유 액터의 서브오브젝트라 ASC에 자동 등록됨)
    AbilitySystemComponent = CreateDefaultSubobject<UCombatAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    AttributeSet = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("AttributeSet"));

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

    // 스태틱 메시 무기 컴포넌트(손 소켓에 부착될 예정) + 강화 오라 VFX
    WeaponStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponStaticMesh"));
    WeaponStaticMesh->SetupAttachment(GetMesh());
    WeaponStaticMesh->SetCollisionProfileName(TEXT("NoCollision"));

    WeaponAuraVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WeaponAuraVFX"));
    WeaponAuraVFX->SetupAttachment(GetMesh());
    WeaponAuraVFX->SetAutoActivate(false);

    // 카메라(SpringArm 프로브)가 다른 캐릭터 몸에 붙어 당겨지지 않게 Camera 채널 무시(벽 충돌은 유지)
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);
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

UAbilitySystemComponent* ACharacterBase::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ACharacterBase::Multicast_HitFeedback_Implementation(UNiagaraSystem* VFX, FVector Location, FVector Normal,
    AActor* Victim, int32 Damage, bool bCritical)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    // 1) 히트 VFX
    if (VFX != nullptr)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, VFX, Location, Normal.Rotation());
    }

    // 2) 피격자 메시 흰색 플래시(오버레이 머티리얼 → 짧게 후 해제)
    if (HitFlashMaterial != nullptr)
    {
        if (ACharacter* VictimChar = Cast<ACharacter>(Victim))
        {
            if (USkeletalMeshComponent* VictimMesh = VictimChar->GetMesh())
            {
                VictimMesh->SetOverlayMaterial(HitFlashMaterial);
                TWeakObjectPtr<USkeletalMeshComponent> WeakMesh = VictimMesh;
                FTimerHandle FlashTimer;
                World->GetTimerManager().SetTimer(FlashTimer,
                    FTimerDelegate::CreateWeakLambda(this, [WeakMesh]()
                    {
                        if (WeakMesh.IsValid())
                        {
                            WeakMesh->SetOverlayMaterial(nullptr);
                        }
                    }),
                    HitFlashDuration, false);
            }
        }
    }

    // 3) 데미지 넘버(각 클라의 로컬 플레이어 화면에만)
    if (Damage > 0)
    {
        APlayerController* PC = World->GetFirstPlayerController();
        if (PC != nullptr && PC->IsLocalController())
        {
            if (UDamageNumberWidget* W = CreateWidget<UDamageNumberWidget>(PC, UDamageNumberWidget::StaticClass()))
            {
                W->AddToViewport(100);
                W->Init(Damage, bCritical ? EDamageType::Critical : EDamageType::Normal, Location);
            }
        }
    }
}

void ACharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    // 서버: 컨트롤러 지정 시 ASC 액터 정보 + 기본 어빌리티/스탯 부여
    InitAbilitySystem();
}

void ACharacterBase::InitAbilitySystem()
{
    if (AbilitySystemComponent == nullptr)
    {
        return;
    }

    // owner=this, avatar=this (Pawn 소유 ASC). 서버/클라 모두 호출 안전(내부에서 갱신).
    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    if (!HasAuthority() || bAbilitiesGranted)
    {
        return;
    }

    // 스탯 초기화 GE 적용(있을 때)
    if (DefaultAttributeEffect)
    {
        FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
        Ctx.AddSourceObject(this);
        FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributeEffect, 1.f, Ctx);
        if (Spec.IsValid())
        {
            AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    }

    // SP 자동 회복 GE 적용(무한 주기)
    if (SPRegenEffect)
    {
        FGameplayEffectContextHandle RegenCtx = AbilitySystemComponent->MakeEffectContext();
        RegenCtx.AddSourceObject(this);
        FGameplayEffectSpecHandle RegenSpec = AbilitySystemComponent->MakeOutgoingSpec(SPRegenEffect, 1.f, RegenCtx);
        if (RegenSpec.IsValid())
        {
            AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*RegenSpec.Data.Get());
        }
    }

    // 기본 어빌리티 부여
    for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
    {
        if (AbilityClass)
        {
            AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
        }
    }
    bAbilitiesGranted = true;
}

void ACharacterBase::BeginPlay()
{
    Super::BeginPlay();

    // 클라이언트/스탠드얼론: 아바타 정보 초기화 (서버는 PossessedBy에서 처리됨)
    InitAbilitySystem();

    // 상태이상 둔화/스턴 반영용 — 평상시 기준 속도 캡처 + 태그 변화 구독
    if (GetCharacterMovement() != nullptr)
    {
        BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
    }
    if (AbilitySystemComponent != nullptr)
    {
        AbilitySystemComponent->RegisterGameplayTagEvent(StudyTags::Status_Chilled, EGameplayTagEventType::NewOrRemoved)
            .AddUObject(this, &ACharacterBase::OnMoveStatusTagChanged);
        AbilitySystemComponent->RegisterGameplayTagEvent(StudyTags::Status_Shocked, EGameplayTagEventType::NewOrRemoved)
            .AddUObject(this, &ACharacterBase::OnMoveStatusTagChanged);
    }

    // 데칼(보스 워닝 데칼 등)이 플레이어 메시에 묻지 않도록 — 데칼은 바닥에만
    USkeletalMeshComponent* MeshParts[] = {
        GetMesh(), HeadMesh, BodyMesh, HandsMesh, LegsMesh, FeetMesh, ShoulderMesh, ArmsMesh, WeaponMesh, ShieldMesh };
    for (USkeletalMeshComponent* Part : MeshParts)
    {
        if (Part != nullptr)
        {
            Part->SetReceivesDecals(false);
        }
    }
    if (WeaponStaticMesh != nullptr)
    {
        WeaponStaticMesh->SetReceivesDecals(false);
    }

    if (HasAuthority())
    {
        LoadCharacter();
    }
}

void ACharacterBase::OnMoveStatusTagChanged(const FGameplayTag /*Tag*/, int32 /*NewCount*/)
{
    RefreshMoveSpeed();
}

void ACharacterBase::SetBaseWalkSpeed(float NewBaseSpeed)
{
    BaseWalkSpeed = NewBaseSpeed;
    RefreshMoveSpeed();
}

void ACharacterBase::RefreshMoveSpeed()
{
    UCharacterMovementComponent* Move = GetCharacterMovement();
    if (Move == nullptr || AbilitySystemComponent == nullptr)
    {
        return;
    }

    float Speed = BaseWalkSpeed;
    if (AbilitySystemComponent->HasMatchingGameplayTag(StudyTags::Status_Shocked))
    {
        Speed = 0.f;   // 스턴: 완전 정지
    }
    else if (AbilitySystemComponent->HasMatchingGameplayTag(StudyTags::Status_Chilled))
    {
        Speed = BaseWalkSpeed * ChillSpeedFactor;   // 둔화: 배율 적용
    }
    Move->MaxWalkSpeed = Speed;
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
