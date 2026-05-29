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

    if (CachedInvPanel     && InvComp)  CachedInvPanel->BindToInventory(InvComp);
    if (CachedEnhancePanel && EnhComp)  CachedEnhancePanel->BindToEnhance(EnhComp);
}

void UEnhanceScreenWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void UEnhanceScreenWidget::OnCloseBtnClicked()
{
    DeactivateWidget();
}
