#include "ItemBase.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/InventoryComponent.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"

AItemBase::AItemBase()
{
    bReplicates = true;

    PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    PickupCollision->SetSphereRadius(PickupRadius);
    PickupCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    SetRootComponent(PickupCollision);

    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(PickupCollision);
    MeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
}

void AItemBase::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &AItemBase::OnSphereBeginOverlap);
    }
}

void AItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AItemBase, ItemID);
    DOREPLIFETIME(AItemBase, Quantity);
}

void AItemBase::InitItem(int32 InItemID, int32 InQuantity)
{
    ItemID   = InItemID;
    Quantity = InQuantity;
    ApplyMesh();
}

void AItemBase::OnRep_ItemID()
{
    ApplyMesh();
}

void AItemBase::ApplyMesh()
{
    if (ItemID <= 0)
    {
        return;
    }

    UGameInstance* GI = GetGameInstance();
    if (GI == nullptr)
    {
        return;
    }

    UItemSubsystem* ItemSub = GI->GetSubsystem<UItemSubsystem>();
    if (ItemSub == nullptr)
    {
        return;
    }

    const FItemData* Data = ItemSub->GetItemData(ItemID);
    if (Data == nullptr || Data->ItemMesh.IsNull())
    {
        return;
    }

    USkeletalMesh* Mesh = Data->ItemMesh.LoadSynchronous();
    if (Mesh && MeshComponent)
    {
        MeshComponent->SetSkeletalMesh(Mesh);
    }
}

void AItemBase::OnSphereBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (!HasAuthority()) return;

    ACharacter* Character = Cast<ACharacter>(OtherActor);
    if (Character)
    {
        Server_PickUp(Character);
    }
}

void AItemBase::Server_PickUp_Implementation(ACharacter* Picker)
{
    if (!Picker || Quantity <= 0) return;

    UInventoryComponent* InvComp = Picker->FindComponentByClass<UInventoryComponent>();
    if (!InvComp) return;

    if (InvComp->IsFull())
    {
        // TODO: 클라이언트에 "인벤토리가 가득 찼습니다" 알림 전송
        return;
    }

    if (InvComp->AddItem(ItemID, Quantity))
    {
        Destroy();
    }
}
