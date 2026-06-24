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
            ActiveSkill = Comp->ConsumePendingActivation(PendingOrigin, PendingDirection);
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
            if (ASkillProjectile* Proj = World->SpawnActor<ASkillProjectile>(ASkillProjectile::StaticClass(), Muzzle, PendingDirection.Rotation(), Params))
            {
                Proj->InitProjectile(ActiveSkill, Avatar, GetAbilitySystemComponentFromActorInfo(), PendingDirection);
            }
        }
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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

    if (ActiveSkill->ExecutionMode == ESkillExecutionMode::Simultaneous)
    {
        RunModulesSimultaneous();
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    SequentialIndex = 0;
    RunNextSequentialModule();
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

void UGA_SkillExecutor::RunModulesSimultaneous()
{
    if (ActiveSkill == nullptr)
    {
        return;
    }

    for (UEffectModule* Module : ActiveSkill->EffectModules)
    {
        if (Module != nullptr)
        {
            Module->Execute(ExecContext);
        }
    }
}

void UGA_SkillExecutor::RunNextSequentialModule()
{
    if (ActiveSkill == nullptr || SequentialIndex >= ActiveSkill->EffectModules.Num())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    UEffectModule* Module = ActiveSkill->EffectModules[SequentialIndex];
    SequentialIndex++;

    if (Module != nullptr)
    {
        Module->Execute(ExecContext);
    }

    if (SequentialIndex >= ActiveSkill->EffectModules.Num())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        const float Step = FMath::Max(0.01f, ActiveSkill->SequentialStep);
        World->GetTimerManager().SetTimer(SequentialTimer, this, &UGA_SkillExecutor::RunNextSequentialModule, Step, false);
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

void UGA_SkillExecutor::ExecuteSkillBurstAt(UWorld* World, USkillDefinition* Skill,
    UAbilitySystemComponent* InstigatorASC, AActor* Instigator,
    UGameplayAbility* SourceAbility, const FVector& Origin, const FVector& Direction)
{
    if (World == nullptr || Skill == nullptr)
    {
        return;
    }

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
    UKismetSystemLibrary::SphereOverlapActors(World, Origin, Skill->Radius, ObjectTypes,
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
                Scale *= Skill->Radius / Skill->VFXReferenceRadius;
            }
            CharBase->Multicast_SpawnSkillVFX(Skill->ImpactVFX, Origin, Direction, Scale);
        }
    }

    for (UEffectModule* Module : Skill->EffectModules)
    {
        if (Module != nullptr)
        {
            Module->Execute(Ctx);
        }
    }
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
        World->GetTimerManager().ClearTimer(SequentialTimer);
        World->GetTimerManager().ClearTimer(FieldTimer);
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
