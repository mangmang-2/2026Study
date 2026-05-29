#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractionPromptComponent.generated.h"

class UWidgetComponent;
class UInteractionPromptWidget;

// 상호작용 대상(NPC·아이템)에 부착 — 플레이어가 포커스하면 프롬프트 표시
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STUDYPROJECT_API UInteractionPromptComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UInteractionPromptComponent();

    void ShowPrompt(const FText& PromptText);
    void HidePrompt();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<UWidgetComponent> WidgetComp;
};
