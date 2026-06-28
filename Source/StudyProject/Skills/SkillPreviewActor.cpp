#include "SkillPreviewActor.h"
#include "SkillDefinition.h"
#include "Modules/PullEffectModule.h"
#include "Modules/PushEffectModule.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "DrawDebugHelpers.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"

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

    // 데모 큐브는 레벨 상주 액터 — 파괴하지 않고 참조만 해제(초기화 버튼이 원위치 복원)
    PropCubes.Reset();

    Super::Destroyed();
}

void ASkillPreviewActor::InitPreview(USkillDefinition* InSkill, const FVector& InCenter, const FVector& InCasterLoc)
{
    Skill = InSkill;
    Center = InCenter;
    CasterLoc = InCasterLoc;
    Center.Z = GroundZ(Center);
    SetActorLocation(Center);

    // 범위 표시 — 디버그 스피어 대신 바닥 원형 데칼(항상 원형 머티리얼 사용)
    if (Skill != nullptr)
    {
        UMaterialInterface* DecalMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Skills/Tools/M_SkillRangeDecal.M_SkillRangeDecal"));
        if (DecalMat != nullptr)
        {
            const float Rad = FMath::Max(20.f, Skill->Radius);
            UGameplayStatics::SpawnDecalAttached(DecalMat, FVector(128.f, Rad * 1.1f, Rad * 1.1f),
                GetRootComponent(), NAME_None, Center, FRotator(-90.f, 0.f, 0.f),
                EAttachLocation::KeepWorldPosition, 0.f);
        }
    }

    if (Skill != nullptr && Skill->CastVFX != nullptr)
    {
        SpawnVFX(Skill->CastVFX, CasterLoc, 1.f, true);
    }

    // 당기기/밀기 모듈이 있으면 타격 주변에 데모 큐브 링을 미리 띄워둔다
    SpawnKnockProps();

    if (Skill == nullptr || Skill->CastTime <= 0.f)
    {
        bCastDone = true;
        BeginDelivery();
    }
}

void ASkillPreviewActor::SpawnKnockProps()
{
    UWorld* World = GetWorld();
    if (Skill == nullptr || World == nullptr)
    {
        return;
    }

    bool bPull = false;
    bool bPush = false;
    float PullStr = 0.f;
    float PushStr = 0.f;
    bool bPullToOrigin = false;
    bool bPushFromOrigin = true;
    for (UEffectModule* Module : Skill->EffectModules)
    {
        if (UPullEffectModule* PullMod = Cast<UPullEffectModule>(Module))
        {
            bPull = true;
            PullStr = PullMod->PullStrength;
            bPullToOrigin = PullMod->bPullToOrigin;
        }
        else if (UPushEffectModule* PushMod = Cast<UPushEffectModule>(Module))
        {
            bPush = true;
            PushStr = PushMod->PushStrength;
            bPushFromOrigin = PushMod->bPushFromOrigin;
        }
    }

    if (bPull == false && bPush == false)
    {
        return;
    }

    bHasKnock = true;
    bPullMode = bPull;   // 둘 다면 당기기 우선

    const float EffectRadius = FMath::Max(20.f, Skill->Radius);
    const float Strength = bPullMode ? PullStr : PushStr;  // 힘 값
    const float KnockScale = 0.18f;                         // 힘 → 프리뷰 변위 환산

    // 기준점: Origin기준이면 폭발 중심(Center), 아니면 시전자(CasterLoc)
    const bool bFromOrigin = bPullMode ? bPullToOrigin : bPushFromOrigin;
    const FVector Anchor = bFromOrigin ? Center : CasterLoc;

    // 레벨에 미리 놓인 데모 큐브(태그 SkillPreviewProp)를 가져다 쓴다 — 재생 때 스폰하지 않음
    for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
    {
        AStaticMeshActor* Cube = *It;
        if (Cube == nullptr || Cube->ActorHasTag(TEXT("SkillPreviewProp")) == false)
        {
            continue;
        }

        const FVector P = Cube->GetActorLocation();

        // 범위(Radius) 밖 큐브는 제외 — 범위 안에서만 밀기/당기기
        FVector ToCenter = P - Center;
        ToCenter.Z = 0.f;
        if (ToCenter.Size() > EffectRadius)
        {
            continue;
        }

        // 변위는 힘 값에 비례(0이면 안 움직임), 방향은 기준점(Anchor) 기준
        FVector Target = P;
        if (bPullMode)
        {
            FVector Dir = Anchor - P;        // 기준점 쪽으로 당김
            Dir.Z = 0.f;
            const float DistToAnchor = Dir.Size();
            Dir = Dir.GetSafeNormal();
            const float Move = FMath::Min(DistToAnchor * 0.95f, Strength * KnockScale);
            Target = P + Dir * Move;
        }
        else
        {
            FVector Dir = P - Anchor;        // 기준점 반대로 밀림
            Dir.Z = 0.f;
            Dir = Dir.GetSafeNormal();
            Target = P + Dir * (Strength * KnockScale);
        }
        Target.Z = P.Z;

        PropCubes.Add(Cube);
        PropStart.Add(P);
        PropTarget.Add(Target);
    }
}

void ASkillPreviewActor::UpdateKnockProps()
{
    const float Duration = 0.6f;
    const float Raw = FMath::Clamp((Elapsed - PropT0) / Duration, 0.f, 1.f);
    const float E = 1.f - FMath::Pow(1.f - Raw, 3.f);   // ease-out

    for (int32 i = 0; i < PropCubes.Num(); ++i)
    {
        if (IsValid(PropCubes[i]) && PropStart.IsValidIndex(i) && PropTarget.IsValidIndex(i))
        {
            PropCubes[i]->SetActorLocation(FMath::Lerp(PropStart[i], PropTarget[i], E));
        }
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

        UMaterialInterface* RainDecalMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Skills/Tools/M_SkillRangeDecal.M_SkillRangeDecal"));

        // 당기기/밀기 힘 값(낙하체별 변위에 비례) + 기준점 플래그
        float RainKnockStr = 0.f;
        bool bRainFromOrigin = bPullMode ? false : true;
        for (UEffectModule* M : Skill->EffectModules)
        {
            if (UPullEffectModule* PM = Cast<UPullEffectModule>(M))
            {
                RainKnockStr = PM->PullStrength;
                bRainFromOrigin = PM->bPullToOrigin;
            }
            else if (UPushEffectModule* PuM = Cast<UPushEffectModule>(M))
            {
                RainKnockStr = PuM->PushStrength;
                bRainFromOrigin = PuM->bPushFromOrigin;
            }
        }

        while (RainSpawned < Total && Elapsed >= RainNextTime)
        {
            const float Ang = FMath::FRandRange(0.f, 2.f * PI);
            const float Dist = FMath::Max(20.f, Skill->Radius) * FMath::Sqrt(FMath::FRand());
            FVector P = Center + FVector(FMath::Cos(Ang) * Dist, FMath::Sin(Ang) * Dist, 0.f);
            P.Z = GroundZ(P);
            SpawnVFX(Skill->ImpactVFX, P, StrikeScale, true);

            // 낙하체 착탄 범위 — 원형 데칼(짧게 페이드)
            if (RainDecalMat != nullptr)
            {
                const float SR = FMath::Max(20.f, Skill->RainStrikeRadius);
                UGameplayStatics::SpawnDecalAtLocation(GetWorld(), RainDecalMat,
                    FVector(128.f, SR * 1.1f, SR * 1.1f), P, FRotator(-90.f, 0.f, 0.f), 1.2f);
            }

            // 낙하체 1발이 떨어진 자리(P) 주변 큐브를 그 지점 기준으로 밀기/당기기(객체별)
            if (bHasKnock)
            {
                const float KnockRange = FMath::Max(150.f, Skill->RainStrikeRadius);
                for (AStaticMeshActor* Prop : PropCubes)
                {
                    if (IsValid(Prop) == false)
                    {
                        continue;
                    }
                    FVector CP = Prop->GetActorLocation();
                    FVector Flat = CP - P;
                    Flat.Z = 0.f;
                    const float D = Flat.Size();
                    if (D <= KnockRange)
                    {
                        // 영향(근접)은 낙하 지점 P 기준, 방향은 기준점(P 또는 시전자) 기준
                        const FVector StrikeAnchor = bRainFromOrigin ? P : CasterLoc;
                        FVector Dir = bPullMode ? (StrikeAnchor - CP) : (CP - StrikeAnchor);
                        Dir.Z = 0.f;
                        Dir = Dir.GetSafeNormal();
                        // 가까울수록 + 힘 값에 비례(0이면 안 움직임)
                        const float Amount = (1.f - D / KnockRange) * (RainKnockStr * 0.06f);
                        FVector NewLoc = CP + Dir * Amount;
                        NewLoc.Z = CP.Z;
                        Prop->SetActorLocation(NewLoc);
                    }
                }
            }

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

    // 단일 임팩트(AOE/투사체)는 중심 기준 1회 당김/밀림.
    // 낙하 폭격(Rain)은 낙하체마다 그 지점에서 미는 게 맞아 아래 rain 루프에서 따로 처리한다.
    const bool bRainKnock = (Skill != nullptr && Skill->DeliveryType == ESkillDeliveryType::Rain);
    if (bHasKnock && bRainKnock == false && bDelivered && bPropsTriggered == false)
    {
        bPropsTriggered = true;
        PropT0 = Elapsed;
    }
    if (bPropsTriggered && bRainKnock == false)
    {
        UpdateKnockProps();
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
