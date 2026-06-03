#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "EnemyCharacter.generated.h"

class UCombatAbilitySystemComponent;
class UCombatAttributeSet;
class UGameplayAbility;
class UGameplayEffect;

/**
 * GAS 기반 테스트용 적 캐릭터.
 * 자체 ASC + AttributeSet를 갖고, 피격/사망/넉다운 등 반응 어빌리티를
 * Event.* 게임플레이 이벤트로 트리거한다. (플레이어 콤보가 GE_Damage를 적용)
 */
UCLASS()
class STUDYPROJECT_API AEnemyCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual void PossessedBy(AController* NewController) override;

protected:
    virtual void BeginPlay() override;

    // ASC ActorInfo 초기화 + (서버) 기본 어빌리티/스탯 부여
    void InitAbilitySystem();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UCombatAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<UCombatAttributeSet> AttributeSet;

    // 이벤트 트리거 기반 반응 어빌리티 (HitReact / Death / AirLaunch 등)
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // 스탯 초기화 GE (없으면 AttributeSet 기본값 사용)
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TSubclassOf<UGameplayEffect> DefaultAttributeEffect;

    // 적 전용 시작 체력(>0이면 AttributeSet 기본값 대신 이 값으로 MaxHP/HP 설정). 플레이어엔 영향 없음.
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float StartingMaxHP = 1000.f;

    // AI가 사용할 공격 어빌리티(서버에서 부여). 기본 GA_EnemyAttack.
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<UGameplayAbility> AttackAbilityClass;

    bool bAbilitiesGranted = false;
};
