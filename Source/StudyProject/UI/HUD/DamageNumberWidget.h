#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ItemData.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

/**
 * 타격 위치에 뜨는 데미지 숫자(코드 전용 — WBP 불필요).
 * 월드 위치를 매 프레임 스크린에 투영하고 위로 떠오르며 페이드아웃, 수명 후 자동 제거.
 * 각 클라이언트에서 로컬 생성(멀티: 글로벌 효과 없음).
 */
UCLASS()
class STUDYPROJECT_API UDamageNumberWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 데미지 숫자 표시 시작. Type=Critical이면 크게/노란색, Heal이면 초록.
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void Init(int32 Damage, EDamageType Type, FVector WorldPos);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> DamageText = nullptr;

    FVector WorldLoc = FVector::ZeroVector;
    float Elapsed = 0.f;
    float Lifetime = 0.8f;
    float RiseSpeed = 90.f;   // 스크린 px/s 상승
    bool  bInitialized = false;
};
