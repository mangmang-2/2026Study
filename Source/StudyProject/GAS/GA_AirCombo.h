#pragma once

#include "CoreMinimal.h"
#include "GA_Combo.h"
#include "GA_AirCombo.generated.h"

/**
 * 공중 콤보 어빌리티 — UGA_Combo를 상속해 공중 전용 콤보 배열만 사용.
 * Input.AirAttack로 활성화(플레이어가 낙하 중일 때 좌클릭이 이 태그로 라우팅).
 */
UCLASS()
class STUDYPROJECT_API UGA_AirCombo : public UGA_Combo
{
    GENERATED_BODY()

public:
    UGA_AirCombo();

protected:
    virtual const TArray<TObjectPtr<UAnimMontage>>& SelectCombo(const FWeaponComboData& Data) const override;
};
