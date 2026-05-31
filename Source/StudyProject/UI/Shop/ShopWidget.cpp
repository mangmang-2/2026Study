#include "ShopWidget.h"
#include "UI/Common/ItemSlotWidget.h"
#include "UI/Common/ItemDragDropOperation.h"
#include "Inventory/ShopComponent.h"
#include "Subsystem/ItemSubsystem.h"
#include "Character/CharacterBase.h"
#include "Engine/GameInstance.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"

void UShopWidget::SetShopComponent(UShopComponent* InShopComp)
{
    ShopComp = InShopComp;
    RefreshShopList();
}

void UShopWidget::RefreshShopList()
{
    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;
    if (!ShopSlotClass || !ItemSub || !ShopComp.IsValid()) return;

    // 한 줄 [고정크기 슬롯] [이름  가격] 을 만드는 람다
    auto BuildRow = [&](int32 ItemID, int32 Index, bool bBuyback) -> UWidget*
    {
        const FItemData* Data = ItemSub->GetItemData(ItemID);

        UItemSlotWidget* SlotW = CreateWidget<UItemSlotWidget>(this, ShopSlotClass);
        if (!SlotW) return nullptr;
        SlotW->SlotIndex   = Index;
        SlotW->SlotContext = ESlotContext::Shop;
        if (Data) SlotW->SetItemData(*Data, 1, 0);

        if (bBuyback)
        {
            SlotW->OnSlotRightClicked.AddDynamic(this, &UShopWidget::HandleBuybackClicked);
        }
        else
        {
            SlotW->OnSlotRightClicked.AddDynamic(this, &UShopWidget::HandleShopItemClicked);
            SlotW->OnSlotDrop.AddDynamic(this, &UShopWidget::HandleShopSlotDrop);
        }

        USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
        IconBox->SetWidthOverride(56.f);
        IconBox->SetHeightOverride(56.f);
        IconBox->AddChild(SlotW);

        const int32 Price = Data ? (bBuyback ? Data->SellPrice : Data->BuyPrice) : 0;
        const FText NamePart = Data ? Data->ItemName : FText::FromString(TEXT("?"));

        UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>();
        NameText->SetText(NamePart);

        UTextBlock* PriceText = WidgetTree->ConstructWidget<UTextBlock>();
        PriceText->SetText(FText::Format(FText::FromString(TEXT("{0} G")), FText::AsNumber(Price)));

        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Row->AddChildToHorizontalBox(IconBox);

        // 이름은 가운데 남는 공간을 채우고(Fill) → 가격이 오른쪽 끝으로 밀림
        if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText))
        {
            NameSlot->SetVerticalAlignment(VAlign_Center);
            NameSlot->SetPadding(FMargin(10.f, 0.f, 8.f, 0.f));
            NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
        // 가격은 오른쪽 끝 정렬
        if (UHorizontalBoxSlot* PriceSlot = Row->AddChildToHorizontalBox(PriceText))
        {
            PriceSlot->SetVerticalAlignment(VAlign_Center);
            PriceSlot->SetHorizontalAlignment(HAlign_Right);
            PriceSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
        }
        return Row;
    };

    if (ShopList)
    {
        ShopList->ClearChildren();
        const TArray<int32>& Items = ShopComp->GetShopItems();
        for (int32 i = 0; i < Items.Num(); ++i)
        {
            if (UWidget* Row = BuildRow(Items[i], i, false)) ShopList->AddChild(Row);
        }
    }

    if (BuybackList)
    {
        BuybackList->ClearChildren();
        const TArray<int32>& BB = ShopComp->GetBuybackItems();
        for (int32 i = 0; i < BB.Num(); ++i)
        {
            if (UWidget* Row = BuildRow(BB[i], i, true)) BuybackList->AddChild(Row);
        }
    }

    if (GoldText)
    {
        ACharacterBase* PC = Cast<ACharacterBase>(GetOwningPlayerPawn());
        const int32 Gold = PC ? PC->GetGold() : 0;
        GoldText->SetText(FText::Format(FText::FromString(TEXT("{0} G")), FText::AsNumber(Gold)));
    }
}

void UShopWidget::HandleShopItemClicked(int32 SlotIndex)
{
    if (ShopComp.IsValid()) ShopComp->RequestBuy(SlotIndex);
}

void UShopWidget::HandleBuybackClicked(int32 SlotIndex)
{
    if (ShopComp.IsValid()) ShopComp->RequestBuyback(SlotIndex);
}

void UShopWidget::HandleShopSlotDrop(ESlotContext SourceContext, int32 FromSlot, int32 ToSlot)
{
    // 인벤 아이템을 상점 슬롯 위에 드롭 → 판매
    if (SourceContext == ESlotContext::Inventory && ShopComp.IsValid())
    {
        ShopComp->RequestSell(FromSlot);
    }
}

bool UShopWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    // 인벤 아이템을 상점 패널 빈 영역에 드롭 → 판매
    if (UItemDragDropOperation* Op = Cast<UItemDragDropOperation>(InOperation))
    {
        if (Op->SourceContext == ESlotContext::Inventory && ShopComp.IsValid())
        {
            ShopComp->RequestSell(Op->SourceSlotIndex);
            return true;
        }
    }
    return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}
