#include "SkillPreviewActor.h"
#include "SkillDefinition.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "DrawDebugHelpers.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ASkillPreviewActor::ASkillPreviewActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

bool ASkillPreviewActor::ShouldTickIfViewportsOnly() const
{
    return true;
}

float ASkillPreviewActor::GroundZ(const FVector& Point) const
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return Point.Z;
    }

    FHitResult Hit;
    if (World->LineTraceSingleByChannel(Hit, Point + FVector(0.f, 0.f, 800.f), Point - FVector(0.f, 0.f, 2000.f), ECC_Visibility))
    {
        return Hit.ImpactPoint.Z;
    }
    return Point.Z;
}

UNiagaraComponent* ASkillPreviewActor::SpawnVFX(UNiagaraSystem* System, const FVector& Location, float Scale, bool bAutoDestroy)
{
    if (System == nullptr)
    {
        return nullptr;
    }

    UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, Location, FRotator::ZeroRotator,
        FVector(FMath::Max(0.05f, Scale)), bAutoDestroy, true);
    if (Comp != nullptr)
    {
        SpawnedVFX.Add(Comp);
    }
    return Comp;
}

void ASkillPreviewActor::Destroyed()
{
    for (UNiagaraComponent* Comp : SpawnedVFX)
    {
        if (IsValid(Comp))
        {
            Comp->DestroyComponent();
        }
    }
    SpawnedVFX.Reset();

    Super::Destroyed();
}

void ASkillPreviewActor::InitPreview(USkillDefinition* InSkill, const FVector& InCenter, const FVector& InCasterLoc)
{
    Skill = InSkill;
    Center = InCenter;
    CasterLoc = InCasterLoc;
    Center.Z = GroundZ(Center);
    SetActorLocation(Center);

    if (Skill != nullptr && Skill->CastVFX != nullptr)
    {
        SpawnVFX(Skill->CastVFX, CasterLoc, 1.f, true);
    }

    if (Skill == nullptr || Skill->CastTime <= 0.f)
    {
        bCastDone = true;
        BeginDelivery();
    }
}

void ASkillPreviewActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (Skill == nullptr)
    {
        Destroy();
        return;
    }

    Elapsed += DeltaSeconds;

    DrawDebugSphere(GetWorld(), Center, FMath::Max(20.f, Skill->Radius), 24, FColor::Yellow, false, 0.f, 0, 1.5f);

    if (bCastDone == false)
    {
        if (Elapsed >= Skill->CastTime)
        {
            bCastDone = true;
            BeginDelivery();
        }
        return;
    }

    if (bProjectileActive)
    {
        const FVector Dir = (Center - ProjectilePos).GetSafeNormal();
        const float Step = FMath::Max(300.f, Skill->ProjectileSpeed) * DeltaSeconds;
        ProjectilePos += Dir * Step;
        if (ProjectileComp != nullptr)
        {
            ProjectileComp->SetWorldLocation(ProjectilePos);
        }

        if (FVector::DistSquared(ProjectilePos, Center) <= FMath::Square(Step + 40.f))
        {
            bProjectileActive = false;
            if (ProjectileComp != nullptr)
            {
                ProjectileComp->DestroyComponent();
                ProjectileComp = nullptr;
            }

            float ImpactScale = Skill->ImpactVFXScale;
            if (Skill->VFXReferenceRadius > 0.f)
            {
                ImpactScale *= Skill->Radius / Skill->VFXReferenceRadius;
            }
            SpawnVFX(Skill->ImpactVFX, Center, ImpactScale, true);
            bDelivered = true;
            FinishTime = Elapsed + 2.f;
        }
    }

    if (bRainActive)
    {
        const int32 Total = FMath::Clamp(Skill->RainStrikeCount, 1, 24);
        const float Window = FMath::Max(0.2f, Skill->RainDuration);
        const float Interval = Window / Total;

        float StrikeScale = Skill->ImpactVFXScale;
        if (Skill->VFXReferenceRadius > 0.f)
        {
            StrikeScale *= Skill->RainStrikeRadius / Skill->VFXReferenceRadius;
        }

        while (RainSpawned < Total && Elapsed >= RainNextTime)
        {
            const float Ang = FMath::FRandRange(0.f, 2.f * PI);
            const float Dist = FMath::Max(20.f, Skill->Radius) * FMath::Sqrt(FMath::FRand());
            FVector P = Center + FVector(FMath::Cos(Ang) * Dist, FMath::Sin(Ang) * Dist, 0.f);
            P.Z = GroundZ(P);
            SpawnVFX(Skill->ImpactVFX, P, StrikeScale, true);
            DrawDebugSphere(GetWorld(), P, FMath::Max(20.f, Skill->RainStrikeRadius), 12, FColor::Orange, false, 1.5f, 0, 1.f);
            RainSpawned++;
            RainNextTime += Interval;
        }

        if (RainSpawned >= Total)
        {
            bRainActive = false;
            bDelivered = true;
            FinishTime = Elapsed + 2.f;
        }
    }

    if (bDelivered && Elapsed >= FinishTime)
    {
        Destroy();
    }
}

void ASkillPreviewActor::BeginDelivery()
{
    if (Skill == nullptr)
    {
        return;
    }

    switch (Skill->DeliveryType)
    {
    case ESkillDeliveryType::Projectile:
    {
        ProjectilePos = CasterLoc + FVector(0.f, 0.f, 60.f);
        ProjectileComp = SpawnVFX(Skill->ProjectileVFX, ProjectilePos, 1.f, false);
        bProjectileActive = true;
        break;
    }
    case ESkillDeliveryType::Rain:
    {
        bRainActive = true;
        RainSpawned = 0;
        RainNextTime = Elapsed;
        break;
    }
    default:
    {
        float ImpactScale = Skill->ImpactVFXScale;
        if (Skill->VFXReferenceRadius > 0.f)
        {
            ImpactScale *= Skill->Radius / Skill->VFXReferenceRadius;
        }
        SpawnVFX(Skill->ImpactVFX, Center, ImpactScale, true);
        bDelivered = true;
        FinishTime = Elapsed + 2.f;
        break;
    }
    }
}
