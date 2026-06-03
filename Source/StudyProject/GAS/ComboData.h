#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ComboData.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class UCameraShakeBase;

/**
 * 타격감(hit-feel) 파라미터. 적중 시 ApplyMeleeDamage가 적용.
 */
USTRUCT(BlueprintType)
struct STUDYPROJECT_API FHitFeel
{
    GENERATED_BODY()

    // 히트스톱: 적중 순간 공격자+피격자를 짧게 정지(지속시간 초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel")
    float HitStopDuration = 0.06f;

    // 히트스톱 시간 배율(0=완전정지 ~ 1=정상). 0.05 = 거의 정지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel")
    float HitStopTimeDilation = 0.05f;

    // 적중 시 카메라 흔들림(없으면 미사용)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel")
    TSubclassOf<UCameraShakeBase> CameraShake;

    // 넉백: 피격자를 뒤로 미는 수평 속도(0=없음)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel")
    float KnockbackSpeed = 0.f;

    // 적중 지점 히트 이펙트(여기에 꽂으면 됨, 비워두면 미사용)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel")
    TObjectPtr<UNiagaraSystem> HitEffect = nullptr;
};

/**
 * 콤보 스텝인(전진키+타겟 방향으로 다가가기) 파라미터.
 * 콤보 몽타주가 제자리(root lock)라 애님 루트모션 대신 코드로 타겟에 접근한다.
 */
USTRUCT(BlueprintType)
struct STUDYPROJECT_API FComboStepIn
{
    GENERATED_BODY()

    // 스텝인 사용 여부
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StepIn")
    bool bEnable = true;

    // 이 거리 안의 타겟에만 스텝인(너무 먼 적까지 끌려가지 않게)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StepIn")
    float MaxRange = 450.f;

    // 타겟에서 이 거리까지 접근(작을수록 더 바짝 붙음)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StepIn")
    float StopDistance = 90.f;

    // 한 타당 접근에 걸리는 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StepIn")
    float Duration = 0.12f;

    // 한 타에서 좁힐 수 있는 최대 거리(넉백으로 밀린 적을 쫓아가려면 크게)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StepIn")
    float MaxStep = 350.f;

    // 타겟을 바라보게 회전하는 속도(0=회전 안 함). 클수록 빨리 돌아봄.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StepIn")
    float RotateSpeed = 15.f;
};

/**
 * 무기별 콤보 데이터(DataTable 행). 행 키 = 무기 ItemID(문자열) 또는 "Default".
 */
USTRUCT(BlueprintType)
struct STUDYPROJECT_API FWeaponComboData : public FTableRowBase
{
    GENERATED_BODY()

    // 지상 콤보(1타~N타 순서)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    TArray<TObjectPtr<UAnimMontage>> GroundCombo;

    // 공중 콤보(1타~N타 순서)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    TArray<TObjectPtr<UAnimMontage>> AirCombo;

    // 런처(공중 띄우기) 몽타주
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    TObjectPtr<UAnimMontage> LauncherMontage = nullptr;

    // 처형(공격자) 몽타주
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    TObjectPtr<UAnimMontage> FinisherMontage = nullptr;

    // 한 타당 데미지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    float DamagePerHit = 25.f;

    // 공격 몽타주 재생 속도(애니 속도). 1.0=기본, 2.0=2배 빠름, 0.5=절반. 콤보/런처/처형 공통.
    // 히트 판정·콤보 윈도우 타이밍도 이 값에 맞춰 자동 보정됨.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float AttackPlayRate = 1.0f;

    // 런처 적중 시 적을 띄우는 상승 속도(높이감). 클수록 높이 뜸.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo|Launch")
    float LaunchEnemyZ = 700.f;

    // 런처 적중 시 플레이어도 따라 뜨는 상승 속도(공중 콤보 연계).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo|Launch")
    float LaunchSelfZ = 600.f;

    // 타격감 파라미터(히트스톱/셰이크/넉백/히트이펙트)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    FHitFeel HitFeel;

    // 콤보 스텝인(전진키로 타겟에 다가가기) 파라미터
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    FComboStepIn StepIn;
};

UCLASS()
class STUDYPROJECT_API UComboLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Avatar의 장착 무기 ItemID로 Table에서 콤보 데이터를 찾는다. 무기 없거나 행 없으면 DefaultRow 사용.
    UFUNCTION(BlueprintCallable, Category = "Combo")
    static bool GetWeaponComboData(AActor* Avatar, UDataTable* Table, FName DefaultRow, FWeaponComboData& OutData);
};
