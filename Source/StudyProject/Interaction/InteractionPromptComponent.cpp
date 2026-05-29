#include "InteractionPromptComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/Common/InteractionPromptWidget.h"

UInteractionPromptComponent::UInteractionPromptComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidgetComp"));
    WidgetComp->SetupAttachment(this);
    WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
    WidgetComp->SetDrawSize(FVector2D(200.f, 60.f));
    WidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    WidgetComp->SetVisibility(false);
    WidgetComp->PrimaryComponentTick.bCanEverTick = false;
}

void UInteractionPromptComponent::BeginPlay()
{
    Super::BeginPlay();

    if (WidgetComp->GetWidgetClass() == nullptr)
    {
        UClass* WClass = LoadClass<UUserWidget>(nullptr,
            TEXT("/Game/UI/Common/WBP_InteractionPrompt.WBP_InteractionPrompt_C"));
        if (WClass)
        {
            WidgetComp->SetWidgetClass(WClass);
        }
    }
    WidgetComp->InitWidget();
}

void UInteractionPromptComponent::ShowPrompt(const FText& PromptText)
{
    if (WidgetComp->GetUserWidgetObject() == nullptr)
    {
        WidgetComp->InitWidget();
    }

    WidgetComp->SetVisibility(true);

    UInteractionPromptWidget* W = Cast<UInteractionPromptWidget>(WidgetComp->GetUserWidgetObject());
    UE_LOG(LogTemp, Warning, TEXT("[Prompt] Widget=%s"), W ? TEXT("OK") : TEXT("NULL"));
    if (W)
    {
        W->SetPromptText(PromptText);
        W->Show();
    }
}

void UInteractionPromptComponent::HidePrompt()
{
    if (UInteractionPromptWidget* W = Cast<UInteractionPromptWidget>(WidgetComp->GetUserWidgetObject()))
        W->Hide();

    WidgetComp->SetVisibility(false);
}
