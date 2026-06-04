#include "BossEnemy.h"
#include "GAS/CombatAttributeSet.h"
#include "UI/HUD/BossHealthBarWidget.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

ABossEnemy::ABossEnemy()
{
    // 보스 기본 체력(BP에서 재설정 가능)
    StartingMaxHP = 5000.f;
    PhaseHealthThresholds = { 0.66f, 0.33f };
    BossHealthBarClass = UBossHealthBarWidget::StaticClass();
}

void ABossEnemy::BeginPlay()
{
    Super::BeginPlay();

    // HP 변화 구독(페이즈 전환). 서버/클라 모두 호출되나 버프는 서버에서만 적용.
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetHPAttribute())
            .AddUObject(this, &ABossEnemy::HandleHPChanged);
    }

    // 보스 체력바 — 각 클라의 로컬 플레이어 화면 상단에 표시
    if (BossHealthBarClass != nullptr)
    {
        if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
        {
            if (PC->IsLocalController())
            {
                BossHealthBarWidget = CreateWidget<UUserWidget>(PC, BossHealthBarClass);
                if (BossHealthBarWidget != nullptr)
                {
                    if (UBossHealthBarWidget* BW = Cast<UBossHealthBarWidget>(BossHealthBarWidget))
                    {
                        BW->SetBoss(this, BossName);
                    }
                    BossHealthBarWidget->AddToViewport(2);
                }
            }
        }
    }
}

void ABossEnemy::HandleHPChanged(const FOnAttributeChangeData& Data)
{
    if (HasAuthority() == false)
    {
        return;   // 페이즈 버프는 서버 권위
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (ASC == nullptr)
    {
        return;
    }
    const float MaxHP = ASC->GetNumericAttribute(UCombatAttributeSet::GetMaxHPAttribute());
    if (MaxHP <= 0.f)
    {
        return;
    }
    const float Frac = Data.NewValue / MaxHP;

    // 임계값을 넘을 때마다 페이즈 상승(여러 단계 한 번에 넘어가도 처리)
    while (CurrentPhase < PhaseHealthThresholds.Num() && Frac <= PhaseHealthThresholds[CurrentPhase])
    {
        ++CurrentPhase;
        // 기준속도 증가 — 둔화/스턴 상태이상이 합성 반영되도록 SetBaseWalkSpeed 경유
        SetBaseWalkSpeed(BaseWalkSpeed + PerPhaseSpeedBonus);
        OnPhaseChanged(CurrentPhase);
    }
}
