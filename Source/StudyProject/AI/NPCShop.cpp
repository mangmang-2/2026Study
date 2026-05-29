#include "NPCShop.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Subsystem/ItemSubsystem.h"
#include "Inventory/InventoryComponent.h"
#include "Character/CharacterBase.h"
#include "Data/ItemData.h"

ANPCShop::ANPCShop()
{
    bHostile  = false;
    bHasShop  = true;

    InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
    InteractSphere->SetupAttachment(RootComponent);
    InteractSphere->SetSphereRadius(InteractRadius);
    InteractSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ANPCShop::BeginPlay()
{
    Super::BeginPlay();

    InteractSphere->SetSphereRadius(InteractRadius);
    InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &ANPCShop::OnInteractBeginOverlap);
    InteractSphere->OnComponentEndOverlap.AddDynamic(this, &ANPCShop::OnInteractEndOverlap);
}

TArray<int32> ANPCShop::GetShopItemList() const
{
    UItemSubsystem* ItemSub = GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return {};
    return ItemSub->GetShopItems(ShopID);
}

void ANPCShop::OpenShopFor(ACharacter* Customer)
{
    // 클라이언트에서 호출 → UI 열기는 PlayerController/HUD에서 처리
    // 상점 아이템 목록은 GetShopItemList()로 전달
}

void ANPCShop::BuyItem(ACharacter* Customer, int32 ItemID, int32 Quantity)
{
    if (!HasAuthority())
    {
        Server_BuyItem(Customer, ItemID, Quantity);
        return;
    }
    Server_BuyItem(Customer, ItemID, Quantity);
}

void ANPCShop::SellItem(ACharacter* Customer, int32 InventorySlot, int32 Quantity)
{
    if (!HasAuthority())
    {
        Server_SellItem(Customer, InventorySlot, Quantity);
        return;
    }
    Server_SellItem(Customer, InventorySlot, Quantity);
}

void ANPCShop::Server_BuyItem_Implementation(ACharacter* Customer, int32 ItemID, int32 Quantity)
{
    if (!Customer) return;

    UItemSubsystem* ItemSub = GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return;

    const FItemData* Data = ItemSub->GetItemData(ItemID);
    if (!Data) return;

    int32 TotalCost = Data->BuyPrice * Quantity;

    // 골드 검증
    if (GetCustomerGold(Customer) < TotalCost) return;

    // 인벤 공간 검증
    ACharacterBase* CharBase = Cast<ACharacterBase>(Customer);
    if (!CharBase) return;

    UInventoryComponent* InvComp = CharBase->GetInventoryComponent();
    if (!InvComp || InvComp->IsFull()) return;

    // 거래 실행
    DeductGold(Customer, TotalCost);
    InvComp->AddItem(ItemID, Quantity);
}

void ANPCShop::Server_SellItem_Implementation(ACharacter* Customer, int32 InventorySlot, int32 Quantity)
{
    if (!Customer) return;

    ACharacterBase* CharBase = Cast<ACharacterBase>(Customer);
    if (!CharBase) return;

    UInventoryComponent* InvComp = CharBase->GetInventoryComponent();
    if (!InvComp) return;

    const FInventorySlot& Slot = InvComp->GetSlot(InventorySlot);
    if (Slot.IsEmpty()) return;

    UItemSubsystem* ItemSub = GetGameInstance()->GetSubsystem<UItemSubsystem>();
    if (!ItemSub) return;

    const FItemData* Data = ItemSub->GetItemData(Slot.ItemID);
    if (!Data) return;

    int32 SellGold = Data->SellPrice * Quantity;

    InvComp->RemoveItem(InventorySlot, Quantity);
    AddGold(Customer, SellGold);
}

void ANPCShop::OnInteractBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    // 상호작용 프롬프트 표시 — BP에서 구현하거나 InteractionPromptWidget 사용
}

void ANPCShop::OnInteractEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, int32)
{
    // 프롬프트 숨기기
}

// ── Gold 헬퍼 ─────────────────────────────────────────────────────────────────

int32 ANPCShop::GetCustomerGold(ACharacter* Customer) const
{
    ACharacterBase* CharBase = Cast<ACharacterBase>(Customer);
    return CharBase ? CharBase->GetGold() : 0;
}

bool ANPCShop::DeductGold(ACharacter* Customer, int32 Amount)
{
    ACharacterBase* CharBase = Cast<ACharacterBase>(Customer);
    return CharBase ? CharBase->SpendGold(Amount) : false;
}

void ANPCShop::AddGold(ACharacter* Customer, int32 Amount)
{
    ACharacterBase* CharBase = Cast<ACharacterBase>(Customer);
    if (CharBase) CharBase->AddGold(Amount);
}
