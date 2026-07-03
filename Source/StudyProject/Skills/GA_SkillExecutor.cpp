#include "GA_SkillExecutor.h"
#include "SkillDefinition.h"
#include "SkillManagerComponent.h"
#include "SkillProjectile.h"
#include "EffectModule.h"
#include "GAS/StudyGameplayTags.h"
#include "Character/CharacterBase.h"
#include "Character/EnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"

UGA_SkillExecutor::UGA_SkillExecutor()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    bLocksMovement = true;

    // 시전 중 캐스트바 트리거 + AI/이동 정지용
    ActivationOwnedTags.AddTag(StudyTags::State_Casting);
    ActivationOwnedTags.AddTag(StudyTags::State_Attacking);

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_Skill);
    SetAssetTags(Tags);
}

void UGA_SkillExecutor::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());

    // 컴포넌트가 활성화 직전(서버) 쌓아둔 스킬·타겟을 읽어온다
    if (Avatar != nullptr)
    {
        if (USkillManagerComponent* Comp = Avatar->FindComponentByClass<USkillManagerComponent>())
        {
            AActor* HomingTarget = nullptr;
            ActiveSkill = Comp->ConsumePendingActivation(PendingOrigin, PendingDirection, HomingTarget);
            PendingTarget = HomingTarget;
        }
    }

    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false || Avatar == nullptr || ActiveSkill == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UWorld* World = Avatar->GetWorld();
    if (World == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // NoTarget이면 시전자 기준
    if (ActiveSkill->TargetingMode == ESkillTargetingMode::NoTarget)
    {
        PendingOrigin = Avatar->GetActorLocation();
        PendingDirection = Avatar->GetActorForwardVector().GetSafeNormal2D();
    }
    else if (PendingDirection.IsNearlyZero() == false)
    {
        // 시전 방향을 바라보게 회전
        Avatar->SetActorRotation(FRotator(0.f, PendingDirection.Rotation().Yaw, 0.f));
    }

    if (ActiveSkill->CastMontage != nullptr)
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            ASC->PlayMontage(this, ActivationInfo, ActiveSkill->CastMontage, 1.0f);
        }
    }

    if (ActiveSkill->CastVFX != nullptr)
    {
        if (ACharacterBase* CharBase = Cast<ACharacterBase>(Avatar))
        {
            CharBase->Multicast_SpawnSkillVFX(ActiveSkill->CastVFX, Avatar->GetActorLocation(), PendingDirection, 1.f);
        }
    }

    if (ActiveSkill->CastTime > 0.f)
    {
        World->GetTimerManager().SetTimer(CastTimer, this, &UGA_SkillExecutor::Detonate, ActiveSkill->CastTime, false);
    }
    else
    {
        Detonate();
    }
}

void UGA_SkillExecutor::Detonate()
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Avatar == nullptr || ActiveSkill == nullptr)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    // 투사체형 — 서버에서 투사체를 쏘고 충돌 시 모듈 실행(투사체가 ExecuteSkillBurstAt 호출)
    if (ActiveSkill->DeliveryType == ESkillDeliveryType::Projectile)
    {
        UWorld* World = Avatar->GetWorld();
        if (World != nullptr)
        {
            const FVector Muzzle = Avatar->GetActorLocation() + PendingDirection * 60.f + FVector(0.f, 0.f, 40.f);
            FActorSpawnParameters Params;
            Params.Owner = Avatar;
            Params.Instigator = Avatar;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            // 호밍 대상: 락온이 있으면 그 대상, 없으면 조준 방향 콘에서 자동 획득
            AActor* HomingTarget = PendingTarget.Get();
            if (ActiveSkill->bHoming && HomingTarget == nullptr)
            {
                HomingTarget = AcquireHomingTarget(Avatar, PendingDirection);
            }

            if (ASkillProjectile* Proj = World->SpawnActor<ASkillProjectile>(ASkillProjectile::StaticClass(), Muzzle, PendingDirection.Rotation(), Params))
            {
                Proj->InitProjectile(ActiveSkill, Avatar, GetAbilitySystemComponentFromActorInfo(), PendingDirection, HomingTarget);
            }
        }
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    // 낙하 폭격형 — RainDuration 동안 여러 낙하체를 시간 분산해 떨군다
    if (ActiveSkill->DeliveryType == ESkillDeliveryType::Rain)
    {
        StartRain();
        return;
    }

    // 지속(필드)형 — TickInterval마다 재판정 + 모듈 재실행
    if (ActiveSkill->Duration > 0.f)
    {
        ChannelElapsed = 0.f;
        FieldTick();   // 즉시 첫 틱

        if (UWorld* World = GetWorld())
        {
            const float Interval = FMath::Max(0.05f, ActiveSkill->TickInterval);
            World->GetTimerManager().SetTimer(FieldTimer, this, &UGA_SkillExecutor::FieldTick, Interval, true);
        }
        return;
    }

    RunOneShot();
}

void UGA_SkillExecutor::RebuildContext()
{
    ExecContext = FSkillExecutionContext();
    ExecContext.Instigator = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    ExecContext.InstigatorASC = GetAbilitySystemComponentFromActorInfo();
    ExecContext.SourceAbility = this;
    ExecContext.Origin = PendingOrigin;
    ExecContext.Direction = PendingDirection;

    CollectTargets(ExecContext);
}

void UGA_SkillExecutor::SpawnImpactVFX()
{
    if (ActiveSkill == nullptr || ActiveSkill->ImpactVFX == nullptr)
    {
        return;
    }

    ACharacterBase* CharBase = Cast<ACharacterBase>(GetAvatarActorFromActorInfo());
    if (CharBase == nullptr)
    {
        return;
    }

    // 범위에 비례해 VFX 확대(기준 반경에서 ImpactVFXScale 그대로)
    float Scale = ActiveSkill->ImpactVFXScale;
    if (ActiveSkill->VFXReferenceRadius > 0.f)
    {
        Scale *= ActiveSkill->Radius / ActiveSkill->VFXReferenceRadius;
    }

    CharBase->Multicast_SpawnSkillVFX(ActiveSkill->ImpactVFX, PendingOrigin, PendingDirection, Scale);
}

void UGA_SkillExecutor::RunOneShot()
{
    RebuildContext();
    SpawnImpactVFX();

    if (ActiveSkill->EffectModules.Num() == 0)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    // 각 모듈을 제 StartDelay에 발동(0이면 같이, 다르면 시간차)
    ScheduleModules(GetWorld(), ActiveSkill->EffectModules, ExecContext);
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_SkillExecutor::FieldTick()
{
    if (ActiveSkill == nullptr)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    RebuildContext();
    SpawnImpactVFX();
    RunModulesSimultaneous();

    ChannelElapsed += FMath::Max(0.05f, ActiveSkill->TickInterval);

    // Duration을 넘기면 종료(예: Duration=3, Interval=1 → t=0,1,2,3 네 번 타격)
    if (ChannelElapsed > ActiveSkill->Duration + 0.01f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(FieldTimer);
        }
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void UGA_SkillExecutor::StartRain()
{
    if (ActiveSkill == nullptr)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    RainStrikesRemaining = FMath::Max(1, ActiveSkill->RainStrikeCount);

    // 동시 낙하 — 한 번에 전부(마지막 발에서 RainStrikeTick이 EndAbility)
    if (ActiveSkill->RainDuration <= 0.05f)
    {
        while (RainStrikesRemaining > 0)
        {
            RainStrikeTick();
        }
        return;
    }

    if (UWorld* World = GetWorld())
    {
        const float Interval = FMath::Max(0.03f, ActiveSkill->RainDuration / FMath::Max(1, ActiveSkill->RainStrikeCount));
        World->GetTimerManager().SetTimer(RainTimer, this, &UGA_SkillExecutor::RainStrikeTick, Interval, true);
    }

    RainStrikeTick();   // 첫 발 즉시
}

void UGA_SkillExecutor::RainStrikeTick()
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    UWorld* World = (Avatar != nullptr) ? Avatar->GetWorld() : nullptr;
    if (Avatar == nullptr || World == nullptr || ActiveSkill == nullptr)
    {
        if (World != nullptr)
        {
            World->GetTimerManager().ClearTimer(RainTimer);
        }
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    const FVector StrikePoint = PickRainStrikePoint();
    ExecuteSkillBurstAt(World, ActiveSkill, GetAbilitySystemComponentFromActorInfo(), Avatar, this,
        StrikePoint, FVector::DownVector, ActiveSkill->RainStrikeRadius);

    RainStrikesRemaining--;
    if (RainStrikesRemaining <= 0)
    {
        World->GetTimerManager().ClearTimer(RainTimer);
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

FVector UGA_SkillExecutor::PickRainStrikePoint() const
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    UWorld* World = (Avatar != nullptr) ? Avatar->GetWorld() : nullptr;
    if (Avatar == nullptr || World == nullptr || ActiveSkill == nullptr)
    {
        return PendingOrigin;
    }

    const float AreaRadius = FMath::Max(50.f, ActiveSkill->Radius);
    FVector Target = PendingOrigin;
    bool bResolved = false;

    // 범위 내 적 위치 가중
    if (FMath::FRand() < ActiveSkill->RainEnemyBias)
    {
        const bool bAvatarIsEnemy = Avatar->IsA(AEnemyCharacter::StaticClass());

        TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
        TArray<AActor*> ToIgnore;
        ToIgnore.Add(Avatar);
        TArray<AActor*> Overlaps;
        UKismetSystemLibrary::SphereOverlapActors(World, PendingOrigin, AreaRadius, ObjectTypes,
            AActor::StaticClass(), ToIgnore, Overlaps);

        TArray<AActor*> Enemies;
        for (AActor* Cand : Overlaps)
        {
            if (IsHostileValidTarget(Cand, bAvatarIsEnemy))
            {
                Enemies.Add(Cand);
            }
        }

        if (Enemies.Num() > 0)
        {
            AActor* Pick = Enemies[FMath::RandRange(0, Enemies.Num() - 1)];
            const float Jitter = FMath::Min(150.f, ActiveSkill->RainStrikeRadius);
            Target = Pick->GetActorLocation();
            Target += FVector(FMath::FRandRange(-Jitter, Jitter), FMath::FRandRange(-Jitter, Jitter), 0.f);
            bResolved = true;
        }
    }

    // 적이 없거나 무작위 분기 — 분포 디스크 내 균등 무작위
    if (bResolved == false)
    {
        const float Angle = FMath::FRandRange(0.f, 2.f * PI);
        const float Dist = AreaRadius * FMath::Sqrt(FMath::FRand());
        Target = PendingOrigin + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
    }

    // 지면 투영(위→아래 트레이스)
    const FVector TraceStart = Target + FVector(0.f, 0.f, 1200.f);
    const FVector TraceEnd = Target - FVector(0.f, 0.f, 2000.f);
    FHitResult GroundHit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Avatar);
    if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params))
    {
        Target.Z = GroundHit.ImpactPoint.Z;
    }
    else
    {
        Target.Z = PendingOrigin.Z;
    }

    return Target;
}

void UGA_SkillExecutor::RunModulesSimultaneous()
{
    if (ActiveSkill == nullptr)
    {
        return;
    }

    ScheduleModules(GetWorld(), ActiveSkill->EffectModules, ExecContext);
}

void UGA_SkillExecutor::ScheduleModules(UWorld* World,
    const TArray<TObjectPtr<UEffectModule>>& Modules, const FSkillExecutionContext& Ctx)
{
    AActor* TimerOwner = Ctx.Instigator.Get();

    for (UEffectModule* Module : Modules)
    {
        if (Module == nullptr)
        {
            continue;
        }

        const float Delay = FMath::Max(0.f, Module->StartDelay);

        // 즉발(또는 타이머를 걸 수 없는 경우)
        if (Delay <= 0.f || World == nullptr || TimerOwner == nullptr)
        {
            Module->Execute(Ctx);
            continue;
        }

        // 지연 발동 — Ctx 복사본을 캡처해 GA 수명과 독립적으로 실행, Instigator에 약결속
        TWeakObjectPtr<UEffectModule> WeakModule(Module);
        FSkillExecutionContext CtxCopy = Ctx;
        FTimerHandle Handle;
        World->GetTimerManager().SetTimer(Handle,
            FTimerDelegate::CreateWeakLambda(TimerOwner,
                [WeakModule, CtxCopy]()
                {
                    if (UEffectModule* M = WeakModule.Get())
                    {
                        M->Execute(CtxCopy);
                    }
                }),
            Delay, false);
    }
}

bool UGA_SkillExecutor::IsHostileValidTarget(AActor* Candidate, bool bInstigatorIsEnemy)
{
    if (Candidate == nullptr)
    {
        return false;
    }

    // 같은 팀 제외(적↔적 / 플레이어↔플레이어)
    const bool bCandIsEnemy = Candidate->IsA(AEnemyCharacter::StaticClass());
    if (bCandIsEnemy == bInstigatorIsEnemy)
    {
        return false;
    }

    if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate))
    {
        if (ASC->HasMatchingGameplayTag(StudyTags::State_Dead))
        {
            return false;
        }
    }

    // 래그돌(시체)은 메시가 물리 시뮬 중 — 제외
    if (ACharacter* Char = Cast<ACharacter>(Candidate))
    {
        if (Char->GetMesh() != nullptr && Char->GetMesh()->IsSimulatingPhysics())
        {
            return false;
        }
    }

    return true;
}

bool UGA_SkillExecutor::IsValidTarget(AActor* Candidate, bool bAvatarIsEnemy) const
{
    return IsHostileValidTarget(Candidate, bAvatarIsEnemy);
}

AActor* UGA_SkillExecutor::AcquireHomingTarget(ACharacter* Avatar, const FVector& Direction) const
{
    UWorld* World = (Avatar != nullptr) ? Avatar->GetWorld() : nullptr;
    if (Avatar == nullptr || World == nullptr || ActiveSkill == nullptr)
    {
        return nullptr;
    }

    const FVector Dir = Direction.GetSafeNormal2D();
    if (Dir.IsNearlyZero())
    {
        return nullptr;
    }

    const FVector Start = Avatar->GetActorLocation();
    // HomingMaxAngle = 콘 전체각 → 조준 방향 기준 반각(±) 안의 적만 대상
    const float CosMax = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(ActiveSkill->HomingMaxAngle, 0.f, 180.f) * 0.5f));
    const bool bAvatarIsEnemy = Avatar->IsA(AEnemyCharacter::StaticClass());

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
    TArray<AActor*> ToIgnore;
    ToIgnore.Add(Avatar);
    TArray<AActor*> Overlaps;
    UKismetSystemLibrary::SphereOverlapActors(World, Start, FMath::Max(200.f, ActiveSkill->Range),
        ObjectTypes, AActor::StaticClass(), ToIgnore, Overlaps);

    const float Speed = FMath::Max(1.f, ActiveSkill->ProjectileSpeed);
    const float Accel = FMath::Max(0.f, ActiveSkill->HomingAcceleration);

    // 조준 콘 안에서 "실제로 따라잡을 수 있는" 적 중 가장 정렬된(각이 작은) 적만 고른다.
    // 못 따라잡을 측면 거리면 획득하지 않아 투사체가 그냥 직선으로 간다(어중간한 빗맞음 방지).
    AActor* Best = nullptr;
    float BestCos = CosMax;
    for (AActor* Cand : Overlaps)
    {
        if (IsHostileValidTarget(Cand, bAvatarIsEnemy) == false)
        {
            continue;
        }

        FVector To = Cand->GetActorLocation() - Start;
        To.Z = 0.f;

        const float Along = FVector::DotProduct(To, Dir);
        if (Along <= 0.f)
        {
            continue;   // 등 뒤
        }

        const float Dist = To.Size();
        const float Cos = Along / FMath::Max(1.f, Dist);
        if (Cos < CosMax)
        {
            continue;   // 콘 밖
        }

        // 비행 시간 동안 호밍이 보정할 수 있는 측면 거리(대략) 안쪽만 획득
        const float Perp = FMath::Sqrt(FMath::Max(0.f, Dist * Dist - Along * Along));
        const float TimeToReach = Along / Speed;
        const float MaxLateral = 0.5f * Accel * TimeToReach * TimeToReach;
        if (Perp > MaxLateral)
        {
            continue;   // 호밍으로도 못 맞춤 → 획득 안 함(직선 비행)
        }

        if (Cos >= BestCos)
        {
            BestCos = Cos;
            Best = Cand;
        }
    }

    return Best;
}

void UGA_SkillExecutor::ExecuteSkillBurstAt(UWorld* World, USkillDefinition* Skill,
    UAbilitySystemComponent* InstigatorASC, AActor* Instigator,
    UGameplayAbility* SourceAbility, const FVector& Origin, const FVector& Direction,
    float OverrideRadius)
{
    if (World == nullptr || Skill == nullptr)
    {
        return;
    }

    const float EffectiveRadius = (OverrideRadius > 0.f) ? OverrideRadius : Skill->Radius;

    FSkillExecutionContext Ctx;
    Ctx.Instigator = Instigator;
    Ctx.InstigatorASC = InstigatorASC;
    Ctx.SourceAbility = SourceAbility;
    Ctx.Origin = Origin;
    Ctx.Direction = Direction.GetSafeNormal();

    const bool bEnemy = (Instigator != nullptr) && Instigator->IsA(AEnemyCharacter::StaticClass());

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
    TArray<AActor*> ToIgnore;
    if (Instigator != nullptr)
    {
        ToIgnore.Add(Instigator);
    }
    TArray<AActor*> Overlaps;
    UKismetSystemLibrary::SphereOverlapActors(World, Origin, EffectiveRadius, ObjectTypes,
        AActor::StaticClass(), ToIgnore, Overlaps);

    for (AActor* Cand : Overlaps)
    {
        if (IsHostileValidTarget(Cand, bEnemy))
        {
            Ctx.HitActors.Add(Cand);
        }
    }

    // 착탄 VFX(범위 비례)
    if (Skill->ImpactVFX != nullptr)
    {
        if (ACharacterBase* CharBase = Cast<ACharacterBase>(Instigator))
        {
            float Scale = Skill->ImpactVFXScale;
            if (Skill->VFXReferenceRadius > 0.f)
            {
                Scale *= EffectiveRadius / Skill->VFXReferenceRadius;
            }
            CharBase->Multicast_SpawnSkillVFX(Skill->ImpactVFX, Origin, Direction, Scale);
        }
    }

    // 각 모듈을 제 StartDelay에 발동(낙하체 1발 안에서도 시간차 가능)
    ScheduleModules(World, Skill->EffectModules, Ctx);
}

void UGA_SkillExecutor::CollectTargets(FSkillExecutionContext& Ctx) const
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    UWorld* World = (Avatar != nullptr) ? Avatar->GetWorld() : nullptr;
    if (Avatar == nullptr || World == nullptr || ActiveSkill == nullptr)
    {
        return;
    }

    const bool bAvatarIsEnemy = Avatar->IsA(AEnemyCharacter::StaticClass());
    const FVector AvatarLoc = Avatar->GetActorLocation();
    const FVector Dir = Ctx.Direction.GetSafeNormal2D();

    // 수집 중심/반경 — 전달 방식에 따라 다름
    FVector GatherCenter = Ctx.Origin;
    float GatherRadius = ActiveSkill->Radius;

    switch (ActiveSkill->DeliveryType)
    {
    case ESkillDeliveryType::Cone:
    case ESkillDeliveryType::Beam:
        GatherCenter = AvatarLoc;
        GatherRadius = ActiveSkill->Range;
        break;
    case ESkillDeliveryType::Melee:
        GatherCenter = AvatarLoc + Avatar->GetActorForwardVector() * ActiveSkill->Radius;
        GatherRadius = ActiveSkill->Radius;
        break;
    default:
        // AOE / Projectile / Dash — 판정 지점 기준
        break;
    }

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    TArray<AActor*> ToIgnore;
    ToIgnore.Add(Avatar);

    TArray<AActor*> Overlaps;
    UKismetSystemLibrary::SphereOverlapActors(World, GatherCenter, GatherRadius, ObjectTypes,
        AActor::StaticClass(), ToIgnore, Overlaps);

    const float CosHalf = FMath::Cos(FMath::DegreesToRadians(ActiveSkill->ConeHalfAngle));

    for (AActor* Cand : Overlaps)
    {
        if (IsValidTarget(Cand, bAvatarIsEnemy) == false)
        {
            continue;
        }

        bool bAccept = true;
        FVector ToTarget = Cand->GetActorLocation() - AvatarLoc;
        ToTarget.Z = 0.f;

        if (ActiveSkill->DeliveryType == ESkillDeliveryType::Cone)
        {
            bAccept = FVector::DotProduct(Dir, ToTarget.GetSafeNormal()) >= CosHalf;
        }
        else if (ActiveSkill->DeliveryType == ESkillDeliveryType::Beam)
        {
            const float Along = FVector::DotProduct(ToTarget, Dir);
            const float Perp = (ToTarget - Dir * Along).Size();
            bAccept = (Along >= 0.f && Along <= ActiveSkill->Range && Perp <= ActiveSkill->Radius);
        }
        else if (ActiveSkill->DeliveryType == ESkillDeliveryType::Melee)
        {
            // 전방 반구만
            bAccept = FVector::DotProduct(Dir, ToTarget.GetSafeNormal()) >= 0.f;
        }

        if (bAccept)
        {
            Ctx.HitActors.Add(Cand);
        }
    }
}

void UGA_SkillExecutor::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CastTimer);
        World->GetTimerManager().ClearTimer(FieldTimer);
        World->GetTimerManager().ClearTimer(RainTimer);
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
