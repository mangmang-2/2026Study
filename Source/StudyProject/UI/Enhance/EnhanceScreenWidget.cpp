#include "EnhanceScreenWidget.h"
#include "UI/Enhance/EnhanceWidget.h"
#include "UI/Inventory/InventoryWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EnhanceComponent.h"
#include "GameFramework/Character.h"

void UEnhanceScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CachedEnhancePanel = Cast<UEnhanceWidget>(GetWidgetFromName(TEXT("EnhancePanel")));
    CachedInvPanel     = Cast<UInventoryWidget>(GetWidgetFromName(TEXT("InvPanel")));

    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (!Char) return;

    UInventoryComponent* InvComp = Char->FindComponentByClass<UInventoryComponent>();
    UEnhanceComponent*   EnhComp = Char->FindComponentByClass<UEnhanceComponent>();

    if (CachedInvPanel && InvComp)
    {
        CachedInvPanel->BindToInventory(InvComp);
        // 강화창 인벤은 우클릭=장착 금지 → 우클릭으로 강화 대상 지정
        CachedInvPanel->SetRightClickEquips(false);
        CachedInvPanel->OnItemSelected.AddUniqueDynamic(this, &UEnhanceScreenWidget::HandleInvItemSelected);
    }
    if (CachedEnhancePanel && EnhComp)
    {
        CachedEnhancePanel->BindToEnhance(EnhComp);
        // 열 때마다 이전 강화 대상 초기화(닫았다 켜도 안 남게)
        CachedEnhancePanel->ClearSlots();
    }
}

void UEnhanceScreenWidget::HandleInvItemSelected(int32 SlotIndex)
{
    if (CachedEnhancePanel != nullptr)
    {
        CachedEnhancePanel->OnTargetSlotDrop(SlotIndex);
    }
}

void UEnhanceScreenWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void UEnhanceScreenWidget::OnCloseBtnClicked()
{
    DeactivateWidget();
}
