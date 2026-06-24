#include "SkillProjectile.h"
#include "SkillDefinition.h"
#include "GA_SkillExecutor.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"

ASkillProjectile::ASkillProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(16.f);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    SetRootComponent(Collision);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->InitialSpeed = 2000.f;
    Movement->MaxSpeed = 2000.f;
    Movement->ProjectileGravityScale = 0.f;
    Movement->bRotationFollowsVelocity = true;

    FlightVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlightVFX"));
    FlightVFX->SetupAttachment(Collision);
    FlightVFX->SetAutoActivate(false);
}

void ASkillProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (Collision != nullptr)
    {
        Collision->OnComponentHit.AddDynamic(this, &ASkillProjectile::OnSphereHit);
    }
}

void ASkillProjectile::InitProjectile(USkillDefinition* InSkill, AActor* InInstigator, UAbilitySystemComponent* InASC, const FVector& Dir)
{
    Skill = InSkill;
    InstigatorActor = InInstigator;
    InstigatorASC = InASC;
    ShotDirection = Dir.GetSafeNormal();

    if (InInstigator != nullptr && Collision != nullptr)
    {
        Collision->IgnoreActorWhenMoving(InInstigator, true);
    }

    if (Skill != nullptr)
    {
        if (Collision != nullptr)
        {
            Collision->SetSphereRadius(FMath::Max(2.f, Skill->ProjectileRadius));
        }
        if (Movement != nullptr)
        {
            Movement->InitialSpeed = Skill->ProjectileSpeed;
            Movement->MaxSpeed = Skill->ProjectileSpeed;
            Movement->Velocity = ShotDirection * Skill->ProjectileSpeed;
        }
        if (FlightVFX != nullptr && Skill->ProjectileVFX != nullptr)
        {
            FlightVFX->SetAsset(Skill->ProjectileVFX);
            FlightVFX->Activate(true);
        }
        // 사거리만큼 날고 자동 소멸(닿지 않으면)
        const float Speed = FMath::Max(1.f, Skill->ProjectileSpeed);
        SetLifeSpan(FMath::Clamp(Skill->Range / Speed, 0.2f, 8.f));
    }
}

void ASkillProjectile::OnSphereHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    FVector NormalImpulse, const FHitResult& Hit)
{
    if (bDetonated || HasAuthority() == false)
    {
        return;
    }
    if (OtherActor == this || OtherActor == InstigatorActor.Get())
    {
        return;
    }

    bDetonated = true;

    const FVector ImpactPoint = Hit.ImpactPoint.IsNearlyZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
    UGA_SkillExecutor::ExecuteSkillBurstAt(GetWorld(), Skill, InstigatorASC.Get(), InstigatorActor.Get(),
        nullptr, ImpactPoint, ShotDirection);

    Destroy();
}
