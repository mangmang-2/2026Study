#include "EnhanceWidget.h"
#include "UI/Common/ItemSlotWidget.h"
#include "Inventory/EnhanceComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Subsystem/ItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UEnhanceWidget::NativeConstruct()
{
    Super::NativeConstruct();
    CachedTargetSlotWidget   = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("TargetSlotWidget")));
    CachedMaterialSlotWidget = Cast<UItemSlotWidget>(GetWidgetFromName(TEXT("MaterialSlotWidget")));
    if (EnhanceButton)
    {
        EnhanceButton->OnClicked.AddDynamic(this, &UEnhanceWidget::HandleEnhanceButton);
    }
}

void UEnhanceWidget::NativeDestruct()
{
    Super::NativeDestruct();
    if (EnhanceButton)
    {
        EnhanceButton->OnClicked.RemoveDynamic(this, &UEnhanceWidget::HandleEnhanceButton);
    }
    if (EnhanceComp.IsValid())
    {
        EnhanceComp->OnEnhanceResult.RemoveDynamic(this, &UEnhanceWidget::HandleEnhanceResult);
    }
}


void UEnhanceWidget::BindToEnhance(UEnhanceComponent* InEnhanceComp)
{
    if (EnhanceComp.IsValid())
    {
        EnhanceComp->OnEnhanceResult.RemoveDynamic(this, &UEnhanceWidget::HandleEnhanceResult);
    }

    EnhanceComp = InEnhanceComp;
    if (EnhanceComp.IsValid())
    {
        EnhanceComp->OnEnhanceResult.AddDynamic(this, &UEnhanceWidget::HandleEnhanceResult);
    }
}

void UEnhanceWidget::OnTargetSlotDrop(int32 InvSlot)
{
    TargetSlotIndex = InvSlot;
    RefreshEnhanceInfo();
}

void UEnhanceWidget::OnMaterialSlotDrop(int32 InvSlot)
{
    MaterialSlotIndex = InvSlot;
    RefreshEnhanceInfo();
}

void UEnhanceWidget::RefreshEnhanceInfo()
{
    ACharacter* Char = Cast<ACharacter>(GetOwningPlayerPawn());
    if (Char == nullptr)
    {
        return;
    }

    UInventoryComponent* InvComp = Char->FindComponentByClass<UInventoryComponent>();
    UItemSubsystem* ItemSub = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UItemSubsystem>() : nullptr;
    if (InvComp == nullptr || ItemSub == nullptr)
    {
        return;
    }

    if (TargetSlotIndex < 0)
    {
        return;
    }

    const FInventorySlot& InvSlot = InvComp->GetSlot(TargetSlotIndex);
    if (InvSlot.IsEmpty())
    {
        return;
    }

    const FItemData* Data = ItemSub->GetItemData(InvSlot.ItemID);
    if (CachedTargetSlotWidget != nullptr && Data != nullptr)
    {
        CachedTargetSlotWidget->SetItemData(*Data, InvSlot.Quantity, InvSlot.EnhanceLevel);
    }

    if (LevelText != nullptr)
    {
        LevelText->SetText(FText::Format(FText::FromString(TEXT("+{0}")), InvSlot.EnhanceLevel));
    }

    const FEnhanceRateRow* Rate = EnhanceComp.IsValid() ? EnhanceComp->GetEnhanceRate(InvSlot.EnhanceLevel) : nullptr;
    if (Rate != nullptr)
    {
        if (SuccessRateText != nullptr)
        {
            SuccessRateText->SetText(FText::Format(
                FText::FromString(TEXT("{0}%")), FMath::RoundToInt(Rate->SuccessRate * 100.f)));
        }

        if (GoldCostText != nullptr)
        {
            GoldCostText->SetText(FText::AsNumber(Rate->GoldCost));
        }

        const FItemData* MatData = ItemSub->GetItemData(Rate->MaterialID);
        if (MaterialText != nullptr)
        {
            MaterialText->SetText(MatData
                ? FText::Format(FText::FromString(TEXT("{0} x{1}")), MatData->ItemName, Rate->MaterialCount)
                : FText::GetEmpty());
        }

        if (CachedMaterialSlotWidget != nullptr && MaterialSlotIndex >= 0)
        {
            const FInventorySlot& MatInvSlot = InvComp->GetSlot(MaterialSlotIndex);
            if (MatInvSlot.IsEmpty() == false && MatData != nullptr)
            {
                CachedMaterialSlotWidget->SetItemData(*MatData, MatInvSlot.Quantity, 0);
            }
        }
    }

    if (ResultText != nullptr)
    {
        ResultText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UEnhanceWidget::ShowEnhanceResult(bool bSuccess)
{
    if (ResultText != nullptr)
    {
        ResultText->SetText(bSuccess
            ? FText::FromString(TEXT("강화 성공!"))
            : FText::FromString(TEXT("강화 실패...")));
        ResultText->SetVisibility(ESlateVisibility::Visible);
    }
}

void UEnhanceWidget::ClearSlots()
{
    TargetSlotIndex   = -1;
    MaterialSlotIndex = -1;

    if (CachedTargetSlotWidget != nullptr)
    {
        CachedTargetSlotWidget->ClearSlot();
    }
    if (CachedMaterialSlotWidget != nullptr)
    {
        CachedMaterialSlotWidget->ClearSlot();
    }
    if (LevelText != nullptr)
    {
        LevelText->SetText(FText::GetEmpty());
    }
    if (SuccessRateText != nullptr)
    {
        SuccessRateText->SetText(FText::GetEmpty());
    }
    if (MaterialText != nullptr)
    {
        MaterialText->SetText(FText::GetEmpty());
    }
    if (GoldCostText != nullptr)
    {
        GoldCostText->SetText(FText::GetEmpty());
    }
    if (ResultText != nullptr)
    {
        ResultText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UEnhanceWidget::HandleEnhanceButton()
{
    if (EnhanceComp.IsValid() && TargetSlotIndex >= 0)
    {
        EnhanceComp->TryEnhance(TargetSlotIndex);
    }
}

void UEnhanceWidget::HandleEnhanceResult(bool bSuccess, int32 NewLevel)
{
    ShowEnhanceResult(bSuccess);
    RefreshEnhanceInfo();
}
