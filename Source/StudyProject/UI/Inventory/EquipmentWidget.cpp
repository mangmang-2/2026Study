#include "EquipmentWidget.h"
#include "UI/Common/ItemSlotWidget.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/InventoryActionHelper.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "Components/TextBlock.h"

void UEquipmentWidget::NativeConstruct()
{
    Super::NativeConstruct();
    CachedHeadSlot     = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("HeadSlot")));
    CachedBodySlot     = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("BodySlot")));
    CachedHandsSlot    = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("HandsSlot")));
    CachedLegsSlot     = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("LegsSlot")));
    CachedFeetSlot     = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("FeetSlot")));
    CachedShoulderSlot = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("ShoulderSlot")));
    CachedArmsSlot     = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("ArmsSlot")));
    CachedWeaponSlot   = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("WeaponSlot")));
    CachedShieldSlot   = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("ShieldSlot")));
}

void UEquipmentWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (BoundEquipment.IsValid())
    {
        BoundEquipment->OnEquipmentChanged.RemoveDynamic(this, &UEquipmentWidget::HandleEquipmentChanged);
    }
}

void UEquipmentWidget::BindToEquipment(UEquipmentComponent* EquipComp)
{
    if (BoundEquipment.IsValid())
    {
        BoundEquipment->OnEquipmentChanged.RemoveDynamic(this, &UEquipmentWidget::HandleEquipmentChanged);
    }

    BoundEquipment = EquipComp;
    if (BoundEquipment.IsValid())
    {
        BoundEquipment->OnEquipmentChanged.AddDynamic(this, &UEquipmentWidget::HandleEquipmentChanged);
        RefreshEquipment();
    }
}

void UEquipmentWidget::RefreshEquipment()
{
    if (BoundEquipment.IsValid() == false)
    {
        return;
    }

    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;

    auto SetupSlot = [&](UItemSlotWidget* W, EEquipSlot EquipSlot, ESlotContext Ctx)
    {
        if (W == nullptr)
        {
            return;
        }
        W->SlotContext = Ctx;
        W->SlotIndex   = (int32)EquipSlot;
        W->OnSlotDrop.Clear();
        W->OnSlotRightClicked.Clear();
        W->OnSlotDrop.AddDynamic(this, &UEquipmentWidget::HandleSlotDrop);
        W->OnSlotRightClicked.AddDynamic(this, &UEquipmentWidget::HandleSlotRightClicked);

        int32 ItemID = BoundEquipment->GetEquippedItemID(EquipSlot);
        if (ItemID != 0 && ItemSub != nullptr)
        {
            const FItemData* Data = ItemSub->GetItemData(ItemID);
            if (Data != nullptr)
            {
                W->SetItemData(*Data, 1, 0);
            }
            else
            {
                W->ClearSlot();
            }
        }
        else
        {
            W->ClearSlot();
        }
    };

    SetupSlot(CachedHeadSlot,     EEquipSlot::Head,     ESlotContext::Equipment);
    SetupSlot(CachedBodySlot,     EEquipSlot::Body,     ESlotContext::Equipment);
    SetupSlot(CachedHandsSlot,    EEquipSlot::Hands,    ESlotContext::Equipment);
    SetupSlot(CachedLegsSlot,     EEquipSlot::Legs,     ESlotContext::Equipment);
    SetupSlot(CachedFeetSlot,     EEquipSlot::Feet,     ESlotContext::Equipment);
    SetupSlot(CachedShoulderSlot, EEquipSlot::Shoulder, ESlotContext::Equipment);
    SetupSlot(CachedArmsSlot,     EEquipSlot::Arms,     ESlotContext::Equipment);
    SetupSlot(CachedWeaponSlot,   EEquipSlot::Weapon,   ESlotContext::Equipment);
    SetupSlot(CachedShieldSlot,   EEquipSlot::Shield,   ESlotContext::Equipment);

    RefreshStats();
}

void UEquipmentWidget::RefreshStats()
{
    if (BoundEquipment.IsValid() == false)
    {
        return;
    }

    if (ATKText != nullptr)
    {
        ATKText->SetText(FText::Format(INVTEXT("ATK: {0}"), BoundEquipment->GetTotalBonusATK()));
    }
    if (DEFText != nullptr)
    {
        DEFText->SetText(FText::Format(INVTEXT("DEF: {0}"), BoundEquipment->GetTotalBonusDEF()));
    }
    if (HPText != nullptr)
    {
        HPText->SetText(FText::Format(INVTEXT("HP: {0}"), BoundEquipment->GetTotalBonusHP()));
    }
}

void UEquipmentWidget::HandleEquipmentChanged()
{
    RefreshEquipment();
}

void UEquipmentWidget::HandleSlotDrop(ESlotContext SourceContext, int32 FromSlot, int32 ToSlot)
{
    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (Char == nullptr)
    {
        return;
    }
    // 드롭 대상이 장비 슬롯이므로 To=Equipment, From=드래그 시작 컨텍스트(인벤이면 장착)
    UInventoryActionHelper::HandleDrop(SourceContext, ESlotContext::Equipment, FromSlot, ToSlot, Char);
}

void UEquipmentWidget::HandleSlotRightClicked(int32 SlotIndex)
{
    if (BoundEquipment.IsValid() == false)
    {
        return;
    }
    BoundEquipment->Unequip(static_cast<EEquipSlot>(SlotIndex));
}
