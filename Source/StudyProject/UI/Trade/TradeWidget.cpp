#include "TradeWidget.h"
#include "UI/Common/ItemSlotWidget.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UTradeWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.AddDynamic(this, &UTradeWidget::HandleConfirm);
    }
    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &UTradeWidget::HandleCancel);
    }
}

void UTradeWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.RemoveDynamic(this, &UTradeWidget::HandleConfirm);
    }
    if (CancelButton)
    {
        CancelButton->OnClicked.RemoveDynamic(this, &UTradeWidget::HandleCancel);
    }
    if (TradeComp.IsValid())
    {
        TradeComp->OnTradeUpdated.RemoveDynamic(this, &UTradeWidget::HandleTradeUpdated);
        TradeComp->OnTradeResult.RemoveDynamic(this, &UTradeWidget::HandleTradeResult);
    }
}

void UTradeWidget::BindToTrade(UTradeComponent* InTradeComp)
{
    if (TradeComp.IsValid())
    {
        TradeComp->OnTradeUpdated.RemoveDynamic(this, &UTradeWidget::HandleTradeUpdated);
        TradeComp->OnTradeResult.RemoveDynamic(this, &UTradeWidget::HandleTradeResult);
    }

    TradeComp = InTradeComp;
    if (TradeComp.IsValid())
    {
        TradeComp->OnTradeUpdated.AddDynamic(this, &UTradeWidget::HandleTradeUpdated);
        TradeComp->OnTradeResult.AddDynamic(this, &UTradeWidget::HandleTradeResult);
        RefreshTrade();
    }
}

void UTradeWidget::RefreshTrade()
{
    if (TradeComp.IsValid() == false)
    {
        return;
    }

    bMyConfirmed    = TradeComp->GetMyOffer().bConfirmed;
    bOtherConfirmed = TradeComp->GetPartnerOffer().bConfirmed;

    RefreshOtherPanel();
    RefreshStatus();

    if (MyOfferList != nullptr && TradeSlotClass != nullptr)
    {
        MyOfferList->ClearChildren();
        UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;

        const TArray<FTradeSlot>& MySlots = TradeComp->GetMyOffer().Slots;
        for (int32 i = 0; i < MySlots.Num(); ++i)
        {
            UItemSlotWidget* W = CreateWidget<UItemSlotWidget>(this, TradeSlotClass);
            if (W == nullptr)
            {
                continue;
            }
            W->SlotIndex   = i;
            W->SlotContext = ESlotContext::TradeRegister;

            const FItemData* Data = ItemSub ? ItemSub->GetItemData(MySlots[i].ItemID) : nullptr;
            if (Data != nullptr)
            {
                W->SetItemData(*Data, MySlots[i].Quantity, 0);
            }

            MyOfferList->AddChild(W);
        }
    }
}

void UTradeWidget::RefreshOtherPanel()
{
    if (TradeComp.IsValid() == false || PartnerOfferList == nullptr || TradeSlotClass == nullptr)
    {
        return;
    }

    PartnerOfferList->ClearChildren();
    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;

    const TArray<FTradeSlot>& OtherSlots = TradeComp->GetPartnerOffer().Slots;
    for (int32 i = 0; i < OtherSlots.Num(); ++i)
    {
        UItemSlotWidget* W = CreateWidget<UItemSlotWidget>(this, TradeSlotClass);
        if (W == nullptr)
        {
            continue;
        }
        W->SlotIndex   = i;
        W->SlotContext = ESlotContext::Trade;

        const FItemData* Data = ItemSub ? ItemSub->GetItemData(OtherSlots[i].ItemID) : nullptr;
        if (Data != nullptr)
        {
            W->SetItemData(*Data, OtherSlots[i].Quantity, 0);
        }

        PartnerOfferList->AddChild(W);
    }
}

void UTradeWidget::RefreshStatus()
{
    if (StatusText == nullptr)
    {
        return;
    }

    if (bMyConfirmed && bOtherConfirmed)
    {
        StatusText->SetText(FText::FromString(TEXT("거래 완료")));
    }
    else if (bMyConfirmed)
    {
        StatusText->SetText(FText::FromString(TEXT("상대방 확인 대기 중...")));
    }
    else if (bOtherConfirmed)
    {
        StatusText->SetText(FText::FromString(TEXT("상대방이 확인했습니다")));
    }
    else
    {
        StatusText->SetText(FText::FromString(TEXT("확인 버튼을 누르세요")));
    }
}

void UTradeWidget::OnTradeCompleted()
{
    if (StatusText != nullptr)
    {
        StatusText->SetText(FText::FromString(TEXT("거래 완료!")));
    }
    SetVisibility(ESlateVisibility::Collapsed);
}

void UTradeWidget::OnTradeCancelled(const FString& Reason)
{
    if (StatusText != nullptr)
    {
        StatusText->SetText(FText::Format(
            FText::FromString(TEXT("거래 취소: {0}")), FText::FromString(Reason)));
    }
    SetVisibility(ESlateVisibility::Collapsed);
}

void UTradeWidget::HandleConfirm()
{
    if (TradeComp.IsValid())
    {
        TradeComp->ConfirmTrade();
    }
}

void UTradeWidget::HandleCancel()
{
    if (TradeComp.IsValid())
    {
        TradeComp->CancelTrade();
    }
}

void UTradeWidget::HandleTradeUpdated()
{
    RefreshTrade();
}

void UTradeWidget::HandleTradeResult(bool bSuccess)
{
    if (bSuccess)
    {
        OnTradeCompleted();
    }
    else
    {
        OnTradeCancelled(TEXT("상대방이 취소했습니다"));
    }
}
