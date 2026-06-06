#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "EquipmentWidget.generated.h"

class UEquipmentComponent;
class UItemSlotWidget;
class UTextBlock;
class UButton;

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
    TObjectPtr<UItemSlotWidget> CachedHeadSlot     = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedBodySlot     = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedHandsSlot    = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedLegsSlot     = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedFeetSlot     = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedShoulderSlot = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedArmsSlot     = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedWeaponSlot   = nullptr;
    UPROPERTY()
    TObjectPtr<UItemSlotWidget> CachedShieldSlot   = nullptr;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ATKText = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> DEFText = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> HPText  = nullptr;

    // 장비 세트 버튼(클릭=적용/빈칸이면 저장, Shift+클릭=덮어쓰기). WBP에 없으면 무시.
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> LoadoutBtn1 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> LoadoutBtn2 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> LoadoutBtn3 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LoadoutText1 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LoadoutText2 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LoadoutText3 = nullptr;

    // 활성(현재 선택) 세트 버튼 강조 색 / 비활성 색
    UPROPERTY(EditAnywhere, Category = "Style")
    FLinearColor ActiveLoadoutColor = FLinearColor(1.0f, 0.78f, 0.25f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Style")
    FLinearColor NormalLoadoutColor = FLinearColor(0.22f, 0.22f, 0.25f, 1.0f);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    TWeakObjectPtr<UEquipmentComponent> BoundEquipment;

    UFUNCTION()
    void HandleEquipmentChanged();

    UFUNCTION()
    void HandleSlotDrop(ESlotContext SourceContext, int32 FromSlot, int32 ToSlot);

    UFUNCTION()
    void HandleSlotRightClicked(int32 SlotIndex);

    UFUNCTION()
    void OnLoadout1Clicked();
    UFUNCTION()
    void OnLoadout2Clicked();
    UFUNCTION()
    void OnLoadout3Clicked();

    void HandleLoadoutClicked(int32 Index);
    void RefreshLoadoutButtons();
};
