#include "ShopScreenWidget.h"
#include "UI/Shop/ShopWidget.h"
#include "UI/Inventory/InventoryWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ShopComponent.h"
#include "Character/CharacterBase.h"
#include "GameFramework/Character.h"
#include "Components/TextBlock.h"

void UShopScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ACharacterBase* PC = Cast<ACharacterBase>(GetOwningPlayerPawn());
    if (PC == nullptr) return;

    UInventoryComponent* InvComp = PC->FindComponentByClass<UInventoryComponent>();
    if (InvPanel != nullptr && InvComp != nullptr)
    {
        InvPanel->BindToInventory(InvComp);
    }

    if (UShopComponent* SC = PC->GetShopComponent())
    {
        if (ShopPanel != nullptr)
        {
            ShopPanel->SetShopComponent(SC);
        }
        SC->OnShopChanged.AddUniqueDynamic(this, &UShopScreenWidget::HandleShopChanged);
    }

    RefreshGold();
}

void UShopScreenWidget::NativeDestruct()
{
    if (ACharacterBase* PC = Cast<ACharacterBase>(GetOwningPlayerPawn()))
    {
        if (UShopComponent* SC = PC->GetShopComponent())
        {
            SC->OnShopChanged.RemoveDynamic(this, &UShopScreenWidget::HandleShopChanged);
        }
    }
    Super::NativeDestruct();
}

void UShopScreenWidget::SetShopID(int32 ShopID)
{
    // 서버에 상점 목록 생성 요청(서버 권위). 결과는 OnShopChanged로 갱신됨.
    if (ACharacterBase* PC = Cast<ACharacterBase>(GetOwningPlayerPawn()))
    {
        if (UShopComponent* SC = PC->GetShopComponent())
        {
            SC->RequestOpenShop(ShopID);
        }
    }
}

void UShopScreenWidget::HandleShopChanged()
{
    if (ShopPanel != nullptr)
    {
        ShopPanel->RefreshShopList();
    }
    RefreshGold();
}

void UShopScreenWidget::RefreshGold()
{
    if (GoldText == nullptr) return;

    ACharacterBase* PC = Cast<ACharacterBase>(GetOwningPlayerPawn());
    const int32 Gold = PC ? PC->GetGold() : 0;
    GoldText->SetText(FText::Format(FText::FromString(TEXT("{0} G")), FText::AsNumber(Gold)));
}

void UShopScreenWidget::OnCloseBtnClicked()
{
    DeactivateWidget();
}
