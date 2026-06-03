#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 보스 체력바(코드 전용 — WBP 불필요). 화면 상단 중앙에 이름 + 큰 HP바.
 * NativeTick에서 대상 보스의 ASC HP/MaxHP를 읽어 갱신. 보스 사망/소멸 시 제거.
 */
UCLASS()
class STUDYPROJECT_API UBossHealthBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetBoss(AActor* InBoss, const FText& InName);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

private:
    UPROPERTY()
    TObjectPtr<UProgressBar> HPBar = nullptr;

    UPROPERTY()
    TObjectPtr<UTextBlock> NameText = nullptr;

    TWeakObjectPtr<AActor> Boss;
};
