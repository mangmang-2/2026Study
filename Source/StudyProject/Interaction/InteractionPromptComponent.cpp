#include "InteractionPromptComponent.h"
#include "UI/Common/InteractionPromptWidget.h"
#include "UObject/ConstructorHelpers.h"

UInteractionPromptComponent::UInteractionPromptComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    SetWidgetSpace(EWidgetSpace::Screen);
    SetDrawSize(FVector2D(200.f, 60.f));
    SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    SetVisibility(false);

    // 위젯 클래스를 생성자(CDO)에서 지정 — 런타임 SetWidgetClass보다 안정적으로 초기화됨
    static ConstructorHelpers::FClassFinder<UUserWidget> PromptWBP(
        TEXT("/Game/UI/Common/WBP_InteractionPrompt"));
    if (PromptWBP.Succeeded())
    {
        SetWidgetClass(PromptWBP.Class);
    }
}

void UInteractionPromptComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetWidgetClass() == nullptr)
    {
        UClass* WClass = LoadClass<UUserWidget>(nullptr,
            TEXT("/Game/UI/Common/WBP_InteractionPrompt.WBP_InteractionPrompt_C"));
        if (WClass)
        {
            SetWidgetClass(WClass);
        }
    }
    InitWidget();
}

void UInteractionPromptComponent::ShowPrompt(const FText& PromptText)
{
    if (GetUserWidgetObject() == nullptr)
    {
        InitWidget();
    }

    SetVisibility(true);

    UUserWidget* WObj = GetUserWidgetObject();
    UE_LOG(LogTemp, Warning, TEXT("[Prompt] ShowPrompt: widgetObj=%s, space=%d, compVisible=%d, class=%s"),
        WObj ? TEXT("OK") : TEXT("NULL"),
        (int32)GetWidgetSpace(),
        IsVisible() ? 1 : 0,
        GetWidgetClass() ? *GetWidgetClass()->GetName() : TEXT("NULL"));

    if (UInteractionPromptWidget* W = Cast<UInteractionPromptWidget>(WObj))
    {
        W->SetPromptText(PromptText);
        W->Show();
    }
}

void UInteractionPromptComponent::HidePrompt()
{
    if (UInteractionPromptWidget* W = Cast<UInteractionPromptWidget>(GetUserWidgetObject()))
    {
        W->Hide();
    }
    SetVisibility(false);
}
