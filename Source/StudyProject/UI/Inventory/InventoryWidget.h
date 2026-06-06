#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "Data/SlotContext.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;
class UInventoryGridWidget;
class UTooltipWidget;
class UContextMenuWidget;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSelected, int32, SlotIndex);

UCLASS(Abstract)
class STUDYPROJECT_API UInventoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void BindToInventory(UInventoryComponent* InvComp);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RefreshInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void FilterByType(EItemType Type);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SortItems(ESortMode Mode);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ShowTooltip(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void HideTooltip();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ShowContextMenu(int32 SlotIndex);

    // 우클릭 시 바로 장착할지(메인 인벤=true). 강화/상점 패널은 false로 두면 우클릭=선택 알림만.
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SetRightClickEquips(bool bEnable) { bRightClickEquips = bEnable; }

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnItemSelected OnItemSelected;

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UInventoryGridWidget> GridWidget  = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTooltipWidget> Tooltip     = nullptr;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UContextMenuWidget> ContextMenu = nullptr;

    // 정렬/필터 툴바 버튼 (WBP에 있으면 자동 바인딩)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> FilterAllButton        = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> FilterWeaponButton     = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> FilterArmorButton      = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> FilterConsumableButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SortRarityButton       = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SortNameButton         = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    ESlotContext Context = ESlotContext::Inventory;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    EItemType CurrentFilter = EItemType::All;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    ESortMode CurrentSort = ESortMode::ByRarity;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    float TooltipDelay = 1.0f;   // 마우스를 올리고 툴팁이 뜨기까지 대기 시간(초)

    // 현재 선택된 필터/정렬 버튼 강조 색 / 비활성 색
    UPROPERTY(EditAnywhere, Category = "Style")
    FLinearColor ActiveToolbarColor = FLinearColor(1.0f, 0.78f, 0.25f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Style")
    FLinearColor NormalToolbarColor = FLinearColor(0.22f, 0.22f, 0.25f, 1.0f);

private:
    TWeakObjectPtr<UInventoryComponent> BoundInventory;

    FTimerHandle TooltipTimerHandle;
    int32 PendingTooltipSlot = -1;
    void ShowPendingTooltip();

    // 툴바에서 마지막으로 누른 버튼(0=전체,1=무기,2=방어구,3=소모품,4=희귀도순,5=이름순). 하나만 강조.
    int32 ActiveToolbarIndex = 0;

    bool bRightClickEquips = true;

    UFUNCTION() void HandleSlotHovered(int32 SlotIndex);
    UFUNCTION() void HandleSlotRightClicked(int32 SlotIndex);
    UFUNCTION() void HandleSlotDrop(ESlotContext SourceContext, int32 FromSlot, int32 ToSlot);
    UFUNCTION() void HandleInventoryChanged();

    // 툴바 버튼 핸들러
    UFUNCTION() void OnFilterAllClicked();
    UFUNCTION() void OnFilterWeaponClicked();
    UFUNCTION() void OnFilterArmorClicked();
    UFUNCTION() void OnFilterConsumableClicked();
    UFUNCTION() void OnSortRarityClicked();
    UFUNCTION() void OnSortNameClicked();

    // 현재 필터/정렬에 맞춰 툴바 버튼 강조 갱신
    void RefreshToolbarButtons();
};
