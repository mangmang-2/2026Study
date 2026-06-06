#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CombatAttributeSet.generated.h"

// 표준 GAS 접근자 매크로 (Getter/Setter/Initter/AttributeGetter)
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 전투 캐릭터 공용 AttributeSet.
 * HP/SP/ATK/DEF/Level/EXP/MoveSpeed + 데미지 적용용 meta attribute(Damage).
 * Attribute 수정은 반드시 GameplayEffect를 통해서만 (직접 set 금지).
 */
UCLASS()
class STUDYPROJECT_API UCombatAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UCombatAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_HP)
    FGameplayAttributeData HP;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, HP)

    UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_MaxHP)
    FGameplayAttributeData MaxHP;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxHP)

    UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_SP)
    FGameplayAttributeData SP;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, SP)

    UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_MaxSP)
    FGameplayAttributeData MaxSP;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxSP)

    UPROPERTY(BlueprintReadOnly, Category = "Stat", ReplicatedUsing = OnRep_ATK)
    FGameplayAttributeData ATK;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, ATK)

    UPROPERTY(BlueprintReadOnly, Category = "Stat", ReplicatedUsing = OnRep_DEF)
    FGameplayAttributeData DEF;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, DEF)

    UPROPERTY(BlueprintReadOnly, Category = "Progress", ReplicatedUsing = OnRep_Level)
    FGameplayAttributeData Level;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Level)

    UPROPERTY(BlueprintReadOnly, Category = "Progress", ReplicatedUsing = OnRep_CurrentEXP)
    FGameplayAttributeData CurrentEXP;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, CurrentEXP)

    UPROPERTY(BlueprintReadOnly, Category = "Progress", ReplicatedUsing = OnRep_RequiredEXP)
    FGameplayAttributeData RequiredEXP;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, RequiredEXP)

    UPROPERTY(BlueprintReadOnly, Category = "Stat", ReplicatedUsing = OnRep_MoveSpeed)
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MoveSpeed)

    // meta attribute: GE_Damage가 싣고 PostGameplayEffectExecute에서 HP로 환산 후 0
    UPROPERTY(BlueprintReadOnly, Category = "Meta")
    FGameplayAttributeData Damage;
    ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Damage)

protected:
    UFUNCTION() void OnRep_HP(const FGameplayAttributeData& Old)         { GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, HP, Old); }
    UFUNCTION() void OnRep_MaxHP(const FGameplayAttributeData& Old)      { GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, MaxHP, Old); }
    UFUNCTION() void OnRep_SP(const FGameplayAttributeData& Old)         { GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, SP, Old); }
    UFUNCTION() void OnRep_MaxSP(const FGameplayAttributeData& Old)      { GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, MaxSP, Old); }
    UFUNCTION() void OnRep_ATK(const FGameplayAttributeData& Old)        { GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, ATK, Old); }
    UFUNCTION() void OnRep_DEF(const FGameplayAttributeData& Old)        { GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, DEF, Old); }
    UFUNCTION() void OnRep_Level(const FGameplayAttributeData& Old)      { GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, Level, Old); }
    UFUNCTION() void OnRep_CurrentEXP(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, CurrentEXP, Old); }
    UFUNCTION() void OnRep_RequiredEXP(const FGameplayAttributeData& Old){ GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, RequiredEXP, Old); }
    UFUNCTION() void OnRep_MoveSpeed(const FGameplayAttributeData& Old)  { GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, MoveSpeed, Old); }
};
