#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractionPromptComponent.generated.h"

class UInteractionPromptWidget;

// 상호작용 대상(NPC·아이템)에 부착 — 플레이어가 포커스하면 머리 위 프롬프트 표시
// UWidgetComponent를 직접 상속 (중첩 서브오브젝트 attach 시 Template Mismatch 발생하므로)
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STUDYPROJECT_API UInteractionPromptComponent : public UWidgetComponent
{
    GENERATED_BODY()

public:
    UInteractionPromptComponent();

    void ShowPrompt(const FText& PromptText);
    void HidePrompt();

protected:
    virtual void BeginPlay() override;
};
