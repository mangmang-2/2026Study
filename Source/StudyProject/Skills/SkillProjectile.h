#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkillProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class USkillDefinition;
class UAbilitySystemComponent;

/**
 * 스킬 투사체(DeliveryType=Projectile). 서버가 스폰, 복제되어 전 클라가 비행을 본다.
 * 충돌 시(서버) UGA_SkillExecutor::ExecuteSkillBurstAt로 착탄 지점에 모듈을 실행.
 */
UCLASS()
class STUDYPROJECT_API ASkillProjectile : public AActor
{
    GENERATED_BODY()

public:
    ASkillProjectile();

    void InitProjectile(USkillDefinition* InSkill, AActor* InInstigator, UAbilitySystemComponent* InASC, const FVector& Dir, AActor* InHomingTarget = nullptr);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnSphereHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector NormalImpulse, const FHitResult& Hit);

    // Skill이 클라에 복제되면 트레일 VFX를 붙인다(서버는 InitProjectile에서 직접)
    UFUNCTION()
    void OnRep_Skill();

    // 비행 트레일 세팅 — 서버/클라 공용
    void SetupFlightVFX();

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UNiagaraComponent> FlightVFX;

    UPROPERTY(ReplicatedUsing = OnRep_Skill)
    TObjectPtr<USkillDefinition> Skill = nullptr;

    TWeakObjectPtr<AActor> InstigatorActor;
    TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC;
    FVector ShotDirection = FVector::ForwardVector;

    bool bDetonated = false;
};
