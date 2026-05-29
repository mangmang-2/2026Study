#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "EquipmentWidget.generated.h"

class UEquipmentComponent;
class UItemSlotWidget;
class UTextBlock;

UCLASS(Abstract)
class STUDYPROJECT_API UEquipmentWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void BindToEquipment(UEquipmentComponent* EquipComp);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void RefreshEquipment();

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void RefreshStats();

protected:
    // UserWidget 인스턴스는 BindWidget 대신 NativeConstruct에서 GetWidgetFromName으로 수동 바인딩
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedHeadSlot   = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedBodySlot   = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedWeaponSlot = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedShieldSlot = nullptr;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ATKText = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> DEFText = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> HPText  = nullptr;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    TWeakObjectPtr<UEquipmentComponent> BoundEquipment;

    UFUNCTION()
    void HandleEquipmentChanged();

    UFUNCTION()
    void HandleSlotDrop(int32 FromSlot, int32 ToSlot);

    UFUNCTION()
    void HandleSlotRightClicked(int32 SlotIndex);
};
