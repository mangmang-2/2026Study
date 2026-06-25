#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMenuShellWidget.generated.h"

class UWidgetSwitcher;
class UButton;
class UTextBlock;
class UHorizontalBox;
class UInventoryWidget;
class UEquipmentWidget;
class UEnhanceWidget;
class UWidgetTree;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMenuCloseRequested);

/**
 * 통합 탭 메뉴 셸 (MooresRPGTemplate 스타일).
 * 상단 탭바(인벤토리/장비/강화) + WidgetSwitcher로 한 창에서 전환.
 * 코드 전용 위젯(RebuildWidget) — 기존 InventoryWidget/EquipmentWidget/EnhanceWidget(WBP) 재사용.
 */
UCLASS()
class STUDYPROJECT_API UGameMenuShellWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UGameMenuShellWidget(const FObjectInitializer& Init);

    // 콘텐츠 위젯 생성 + 컴포넌트 바인딩 + 초기 탭 선택. AddToViewport 후 호출.
    UFUNCTION(BlueprintCallable, Category = "UI")
    void InitShell(int32 InitialTab);

    void SelectTab(int32 Index);

    UPROPERTY(BlueprintAssignable)
    FOnMenuCloseRequested OnCloseRequested;

    enum { TAB_Inventory = 0, TAB_Equipment = 1, TAB_Enhance = 2 };

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geo, const FKeyEvent& Key) override;

private:
    // WBP 콘텐츠 클래스 (생성자에서 FClassFinder)
    TSubclassOf<UUserWidget> InvWidgetClass   = nullptr;
    TSubclassOf<UUserWidget> EquipWidgetClass = nullptr;
    TSubclassOf<UUserWidget> EnhanceWidgetClass = nullptr;

    UPROPERTY() TObjectPtr<UWidgetSwitcher> Switcher = nullptr;
    UPROPERTY() TObjectPtr<UHorizontalBox>  TabBar   = nullptr;
    UPROPERTY() TArray<TObjectPtr<UButton>> TabButtons;
    UPROPERTY() TArray<TObjectPtr<UTextBlock>> TabTexts;
    UPROPERTY() TArray<TObjectPtr<class UImage>> TabUnderlines;

    UPROPERTY() TObjectPtr<UInventoryWidget> InvW       = nullptr; // 인벤 탭
    UPROPERTY() TObjectPtr<UEquipmentWidget> EquipW     = nullptr; // 장비 탭
    UPROPERTY() TObjectPtr<UInventoryWidget> EquipInv   = nullptr; // 장비 탭의 인벤(드래그 착용용)
    UPROPERTY() TObjectPtr<UEnhanceWidget>   EnhanceW   = nullptr; // 강화 탭
    UPROPERTY() TObjectPtr<UInventoryWidget> EnhanceInv = nullptr; // 강화 탭의 아이템 선택용 인벤

    UPROPERTY() TObjectPtr<UTextBlock> WeightText = nullptr;
    UPROPERTY() TObjectPtr<UTextBlock> GoldText   = nullptr;

    int32 ActiveTab = 0;
    bool  bContentReady = false;

    UFUNCTION() void OnTabInventory();
    UFUNCTION() void OnTabEquipment();
    UFUNCTION() void OnTabEnhance();
    UFUNCTION() void OnCloseClicked();
    UFUNCTION() void HandleEnhanceItemSelected(int32 SlotIndex);

    void BuildTabBar(UWidgetTree* Tree);
    UButton* AddTab(UWidgetTree* Tree, const FString& Label, int32 Index);
    void RefreshTabVisuals();
};
