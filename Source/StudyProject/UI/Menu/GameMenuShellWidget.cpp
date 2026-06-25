#include "GameMenuShellWidget.h"
#include "MenuUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/BackgroundBlur.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Spacer.h"
#include "Components/Button.h"
#include "Components/SlateWrapperTypes.h"

#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

#include "UI/Inventory/InventoryWidget.h"
#include "UI/Inventory/EquipmentWidget.h"
#include "UI/Enhance/EnhanceWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/EnhanceComponent.h"

UGameMenuShellWidget::UGameMenuShellWidget(const FObjectInitializer& Init)
    : Super(Init)
{
    static ConstructorHelpers::FClassFinder<UUserWidget> InvF(TEXT("/Game/UI/Inventory/WBP_InventoryWidget"));
    if (InvF.Succeeded()) InvWidgetClass = InvF.Class;
    static ConstructorHelpers::FClassFinder<UUserWidget> EqF(TEXT("/Game/UI/Inventory/WBP_EquipmentWidget"));
    if (EqF.Succeeded()) EquipWidgetClass = EqF.Class;
    static ConstructorHelpers::FClassFinder<UUserWidget> EnF(TEXT("/Game/UI/Enhance/WBP_EnhanceWidget"));
    if (EnF.Succeeded()) EnhanceWidgetClass = EnF.Class;

    SetIsFocusable(true);   // ESC/Tab 키 입력 받도록
}

TSharedRef<SWidget> UGameMenuShellWidget::RebuildWidget()
{
    if (WidgetTree == nullptr)
    {
        return Super::RebuildWidget();
    }

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    // 1) 전체 화면 블러 + 살짝 어둡게
    UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
    Blur->SetBlurStrength(3.f);
    if (UOverlaySlot* BS = Root->AddChildToOverlay(Blur))
    {
        BS->SetHorizontalAlignment(HAlign_Fill);
        BS->SetVerticalAlignment(VAlign_Fill);
    }
    UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
    Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.45f));
    Blur->SetContent(Dim);

    // 2) 패널 — 화면을 거의 채움(여백만). 인벤/장비/강화 콘텐츠가 중앙앵커 풀스크린 설계라
    //    충분한 높이가 없으면 상단 툴바가 위로 넘쳐 탭바와 겹친다. 그래서 큰 패널 사용.
    UOverlay* PanelOverlay = WidgetTree->ConstructWidget<UOverlay>();
    if (UOverlaySlot* POSlot = Root->AddChildToOverlay(PanelOverlay))
    {
        POSlot->SetHorizontalAlignment(HAlign_Fill);
        POSlot->SetVerticalAlignment(VAlign_Fill);
        POSlot->SetPadding(FMargin(0.f)); // 화면 전체를 덮음(여백 틈 제거)
    }

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(MenuUI::PanelFill());
    Panel->SetPadding(FMargin(24.f));
    if (UOverlaySlot* PS = PanelOverlay->AddChildToOverlay(Panel))
    {
        PS->SetHorizontalAlignment(HAlign_Fill);
        PS->SetVerticalAlignment(VAlign_Fill);
    }

    // 3) 패널 내부 세로 배치
    UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->SetContent(VB);

    // 3a) 탭바
    TabBar = WidgetTree->ConstructWidget<UHorizontalBox>();
    if (UVerticalBoxSlot* TS = VB->AddChildToVerticalBox(TabBar))
    {
        TS->SetHorizontalAlignment(HAlign_Center);
        TS->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }
    BuildTabBar(WidgetTree);

    // 3b) 상단 장식 줄
    UImage* Line1 = WidgetTree->ConstructWidget<UImage>();
    Line1->SetBrush(MenuUI::LineBrush(MenuUI::DoubleLineTex(), 10.f));
    if (UVerticalBoxSlot* L1 = VB->AddChildToVerticalBox(Line1))
    {
        L1->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
    }

    // 3c) 콘텐츠 스위처 (채움). 위쪽 간격을 크게 줘서 인벤 자체 정렬툴바가 탭바와 겹치지 않게.
    Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
    if (UVerticalBoxSlot* SW = VB->AddChildToVerticalBox(Switcher))
    {
        SW->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        SW->SetPadding(FMargin(0.f, 12.f, 0.f, 12.f));
    }

    // 3d) 하단 장식 줄
    UImage* Line2 = WidgetTree->ConstructWidget<UImage>();
    Line2->SetBrush(MenuUI::LineBrush(MenuUI::HorizLineTex(), 6.f));
    VB->AddChildToVerticalBox(Line2);

    // 3e) 상태바
    UHorizontalBox* Status = WidgetTree->ConstructWidget<UHorizontalBox>();
    if (UVerticalBoxSlot* STS = VB->AddChildToVerticalBox(Status))
    {
        STS->SetPadding(FMargin(4.f, 8.f, 4.f, 0.f));
    }

    WeightText = WidgetTree->ConstructWidget<UTextBlock>();
    WeightText->SetFont(MenuUI::Font(18.f));
    WeightText->SetColorAndOpacity(FSlateColor(MenuUI::TabTextActive()));
    WeightText->SetText(FText::FromString(TEXT("INVENTORY")));
    Status->AddChildToHorizontalBox(WeightText);

    USpacer* Sp = WidgetTree->ConstructWidget<USpacer>();
    Sp->SetSize(FVector2D(40.f, 1.f));
    if (UHorizontalBoxSlot* SpS = Status->AddChildToHorizontalBox(Sp))
    {
        SpS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    GoldText = WidgetTree->ConstructWidget<UTextBlock>();
    GoldText->SetFont(MenuUI::Font(18.f));
    GoldText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.84f, 0.3f, 1.f)));
    GoldText->SetText(FText::FromString(TEXT("GOLD 0")));
    Status->AddChildToHorizontalBox(GoldText);

    USpacer* Sp2 = WidgetTree->ConstructWidget<USpacer>();
    Sp2->SetSize(FVector2D(24.f, 1.f));
    Status->AddChildToHorizontalBox(Sp2);

    UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>();
    CloseBtn->SetStyle(MenuUI::ButtonStyle());
    CloseBtn->OnClicked.AddDynamic(this, &UGameMenuShellWidget::OnCloseClicked);
    UTextBlock* CloseTxt = WidgetTree->ConstructWidget<UTextBlock>();
    CloseTxt->SetText(FText::FromString(TEXT("CLOSE")));
    CloseTxt->SetFont(MenuUI::Font(16.f));
    CloseTxt->SetColorAndOpacity(FSlateColor(MenuUI::TabTextNormal()));
    CloseBtn->SetContent(CloseTxt);
    Status->AddChildToHorizontalBox(CloseBtn);

    // 4) 코너 장식 4개
    auto AddCorner = [&](EHorizontalAlignment H, EVerticalAlignment V, float sx, float sy)
    {
        UImage* C = WidgetTree->ConstructWidget<UImage>();
        if (UTexture2D* T = MenuUI::CornerTex())
        {
            C->SetBrushFromTexture(T);
        }
        C->SetDesiredSizeOverride(FVector2D(56.f, 56.f));
        C->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
        C->SetRenderScale(FVector2D(sx, sy));
        if (UOverlaySlot* CS = PanelOverlay->AddChildToOverlay(C))
        {
            CS->SetHorizontalAlignment(H);
            CS->SetVerticalAlignment(V);
        }
    };
    AddCorner(HAlign_Left,  VAlign_Top,     1.f,  1.f);
    AddCorner(HAlign_Right, VAlign_Top,    -1.f,  1.f);
    AddCorner(HAlign_Left,  VAlign_Bottom,  1.f, -1.f);
    AddCorner(HAlign_Right, VAlign_Bottom, -1.f, -1.f);

    return Super::RebuildWidget();
}

void UGameMenuShellWidget::BuildTabBar(UWidgetTree* Tree)
{
    TabButtons.Reset();
    TabTexts.Reset();
    TabUnderlines.Reset();
    AddTab(Tree, TEXT("INVENTORY"), TAB_Inventory);
    AddTab(Tree, TEXT("EQUIPMENT"), TAB_Equipment);
    AddTab(Tree, TEXT("ENHANCE"),   TAB_Enhance);
}

UButton* UGameMenuShellWidget::AddTab(UWidgetTree* Tree, const FString& Label, int32 Index)
{
    // 각 탭 = [투명 버튼(텍스트)] + [밑줄 강조(활성 시만 표시)]
    UVerticalBox* Cell = Tree->ConstructWidget<UVerticalBox>();

    UButton* B = Tree->ConstructWidget<UButton>();
    B->SetStyle(MenuUI::TabButtonStyle(false));
    switch (Index)
    {
    case TAB_Inventory: B->OnClicked.AddDynamic(this, &UGameMenuShellWidget::OnTabInventory); break;
    case TAB_Equipment: B->OnClicked.AddDynamic(this, &UGameMenuShellWidget::OnTabEquipment); break;
    case TAB_Enhance:   B->OnClicked.AddDynamic(this, &UGameMenuShellWidget::OnTabEnhance);   break;
    default: break;
    }

    UTextBlock* T = Tree->ConstructWidget<UTextBlock>();
    T->SetText(FText::FromString(Label));
    T->SetFont(MenuUI::Font(22.f));
    T->SetColorAndOpacity(FSlateColor(MenuUI::TabTextNormal()));
    B->SetContent(T);
    Cell->AddChildToVerticalBox(B);

    UImage* UL = Tree->ConstructWidget<UImage>();
    {
        FSlateBrush LB;
        LB.DrawAs = ESlateBrushDrawType::RoundedBox;
        LB.TintColor = FSlateColor(FLinearColor(0.96f, 0.90f, 0.70f, 1.f)); // 따뜻한 강조선
        LB.OutlineSettings.CornerRadii = FVector4(1.5f, 1.5f, 1.5f, 1.5f);
        LB.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        LB.ImageSize = FVector2D(16.f, 3.f);
        UL->SetBrush(LB);
    }
    UL->SetVisibility(ESlateVisibility::Hidden);
    if (UVerticalBoxSlot* US = Cell->AddChildToVerticalBox(UL))
    {
        US->SetHorizontalAlignment(HAlign_Fill);
        US->SetPadding(FMargin(12.f, 2.f, 12.f, 0.f));
    }
    TabUnderlines.Add(UL);

    if (UHorizontalBoxSlot* S = TabBar->AddChildToHorizontalBox(Cell))
    {
        S->SetPadding(FMargin(6.f, 2.f));
    }
    TabButtons.Add(B);
    TabTexts.Add(T);
    return B;
}

void UGameMenuShellWidget::InitShell(int32 InitialTab)
{
    if (bContentReady)
    {
        SelectTab(InitialTab);
        return;
    }
    if (Switcher == nullptr)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    ACharacter* Char = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
    UInventoryComponent* InvComp = Char ? Char->FindComponentByClass<UInventoryComponent>() : nullptr;
    UEquipmentComponent* EquipComp = Char ? Char->FindComponentByClass<UEquipmentComponent>() : nullptr;
    UEnhanceComponent* EnhComp = Char ? Char->FindComponentByClass<UEnhanceComponent>() : nullptr;

    // 좌우 균형용 fill 스페이서(묶음을 가운데로 밀어줌)
    auto AddFillSpacer = [this](UHorizontalBox* Box)
    {
        USpacer* Sp = WidgetTree->ConstructWidget<USpacer>();
        if (UHorizontalBoxSlot* S = Box->AddChildToHorizontalBox(Sp))
        {
            S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    };

    // 탭0: 인벤토리(우클릭=장착)
    if (InvWidgetClass)
    {
        InvW = CreateWidget<UInventoryWidget>(PC, InvWidgetClass);
        if (InvW)
        {
            InvW->SetRightClickEquips(true);
            if (InvComp) InvW->BindToInventory(InvComp);
            Switcher->AddChild(InvW);
        }
    }

    // 탭1: 장비( [여백] 장비슬롯 | 인벤그리드 [여백] ) — 두 패널 묶음을 가운데 정렬해 균형
    {
        UHorizontalBox* QHB = WidgetTree->ConstructWidget<UHorizontalBox>();
        AddFillSpacer(QHB);

        if (EquipWidgetClass)
        {
            EquipW = CreateWidget<UEquipmentWidget>(PC, EquipWidgetClass);
            if (EquipW)
            {
                if (EquipComp) EquipW->BindToEquipment(EquipComp);
                // 장비 위젯은 중앙앵커 캔버스라 폭을 명시해야 슬롯이 보인다.
                USizeBox* QBox = WidgetTree->ConstructWidget<USizeBox>();
                QBox->SetWidthOverride(600.f);
                QBox->SetContent(EquipW);
                QHB->AddChildToHorizontalBox(QBox);
            }
        }
        if (InvWidgetClass)
        {
            EquipInv = CreateWidget<UInventoryWidget>(PC, InvWidgetClass);
            if (EquipInv)
            {
                EquipInv->SetRightClickEquips(true);
                if (InvComp) EquipInv->BindToInventory(InvComp);
                USizeBox* IBox = WidgetTree->ConstructWidget<USizeBox>();
                IBox->SetWidthOverride(720.f);
                IBox->SetContent(EquipInv);
                if (UHorizontalBoxSlot* S = QHB->AddChildToHorizontalBox(IBox))
                {
                    S->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
                }
            }
        }
        AddFillSpacer(QHB);
        Switcher->AddChild(QHB);
    }

    // 탭2: 강화( [여백] 강화패널 | 인벤 [여백] ) — 묶음 가운데 정렬
    {
        UHorizontalBox* EHB = WidgetTree->ConstructWidget<UHorizontalBox>();
        AddFillSpacer(EHB);

        if (EnhanceWidgetClass)
        {
            EnhanceW = CreateWidget<UEnhanceWidget>(PC, EnhanceWidgetClass);
            if (EnhanceW)
            {
                if (EnhComp) EnhanceW->BindToEnhance(EnhComp);
                USizeBox* EBox = WidgetTree->ConstructWidget<USizeBox>();
                EBox->SetWidthOverride(500.f);
                EBox->SetContent(EnhanceW);
                EHB->AddChildToHorizontalBox(EBox);
            }
        }
        if (InvWidgetClass)
        {
            EnhanceInv = CreateWidget<UInventoryWidget>(PC, InvWidgetClass);
            if (EnhanceInv)
            {
                EnhanceInv->SetRightClickEquips(false);
                if (InvComp) EnhanceInv->BindToInventory(InvComp);
                EnhanceInv->OnItemSelected.AddDynamic(this, &UGameMenuShellWidget::HandleEnhanceItemSelected);
                USizeBox* EIBox = WidgetTree->ConstructWidget<USizeBox>();
                EIBox->SetWidthOverride(720.f);
                EIBox->SetContent(EnhanceInv);
                if (UHorizontalBoxSlot* S = EHB->AddChildToHorizontalBox(EIBox))
                {
                    S->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
                }
            }
        }
        AddFillSpacer(EHB);
        Switcher->AddChild(EHB);
    }

    bContentReady = true;
    SelectTab(InitialTab);
}

void UGameMenuShellWidget::SelectTab(int32 Index)
{
    ActiveTab = FMath::Clamp(Index, 0, 2);
    if (Switcher)
    {
        Switcher->SetActiveWidgetIndex(ActiveTab);
    }
    RefreshTabVisuals();

    if (ActiveTab == TAB_Enhance && EnhanceW)
    {
        EnhanceW->ClearSlots();
    }
}

void UGameMenuShellWidget::RefreshTabVisuals()
{
    for (int32 i = 0; i < TabButtons.Num(); ++i)
    {
        const bool bActive = (i == ActiveTab);
        if (TabButtons[i]) TabButtons[i]->SetStyle(MenuUI::TabButtonStyle(bActive));
        if (TabTexts.IsValidIndex(i) && TabTexts[i])
        {
            TabTexts[i]->SetColorAndOpacity(FSlateColor(bActive ? MenuUI::TabTextActive() : MenuUI::TabTextNormal()));
        }
        if (TabUnderlines.IsValidIndex(i) && TabUnderlines[i])
        {
            TabUnderlines[i]->SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
        }
    }
}

void UGameMenuShellWidget::OnTabInventory() { SelectTab(TAB_Inventory); }
void UGameMenuShellWidget::OnTabEquipment() { SelectTab(TAB_Equipment); }
void UGameMenuShellWidget::OnTabEnhance()   { SelectTab(TAB_Enhance); }

void UGameMenuShellWidget::OnCloseClicked()
{
    OnCloseRequested.Broadcast();
}

void UGameMenuShellWidget::HandleEnhanceItemSelected(int32 SlotIndex)
{
    if (EnhanceW)
    {
        EnhanceW->OnTargetSlotDrop(SlotIndex);
    }
}

FReply UGameMenuShellWidget::NativeOnKeyDown(const FGeometry& Geo, const FKeyEvent& Key)
{
    const FKey K = Key.GetKey();
    if (K == EKeys::Escape || K == EKeys::Q)
    {
        OnCloseRequested.Broadcast();
        return FReply::Handled();
    }
    if (K == EKeys::Tab)
    {
        SelectTab((ActiveTab + 1) % 3);
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geo, Key);
}
