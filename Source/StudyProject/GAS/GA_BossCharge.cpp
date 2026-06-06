#include "GA_BossCharge.h"
#include "StudyGameplayTags.h"
#include "GE_Damage.h"
#include "Character/EnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

UGA_BossCharge::UGA_BossCharge()
{
    // 적 공격은 서버 권위
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    DamageGEClass = UGE_Damage::StaticClass();
    bLocksMovement = true;

    // 돌진 중 슈퍼아머(피격모션으로 안 끊김)
    ActivationOwnedTags.AddTag(StudyTags::Status_SuperArmor);

    // 돌진 중 보스 정지(움직이면 데칼과 어긋남)
    ActivationOwnedTags.AddTag(StudyTags::State_Attacking);

    FGameplayTagContainer Tags;
    Tags.AddTag(StudyTags::Ability_EnemyAttack);
    SetAssetTags(Tags);
}

void UGA_BossCharge::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false || Avatar == nullptr)
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

    // 발동 순간 방향 고정(이후 추적 없음, 회피 가능)
    ChargeDir = Avatar->GetActorForwardVector().GetSafeNormal2D();
    if (APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0))
    {
        FVector ToPlayer = Player->GetActorLocation() - Avatar->GetActorLocation();
        ToPlayer.Z = 0.f;
        if (ToPlayer.SizeSquared() > 1.f)
        {
            ChargeDir = ToPlayer.GetSafeNormal();
        }
    }

    Avatar->SetActorRotation(FRotator(0.f, ChargeDir.Rotation().Yaw, 0.f));

    // 데칼은 코스메틱 — 멀티캐스트로 전 클라에 표시
    if (WarningDecalMaterial != nullptr)
    {
        if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Avatar))
        {
            const float HalfHeight = Avatar->GetSimpleCollisionHalfHeight();
            const FVector Anchor(Avatar->GetActorLocation().X, Avatar->GetActorLocation().Y,
                Avatar->GetActorLocation().Z - HalfHeight + 2.f);
            // 투영깊이 600(계단/경사 커버)
            Enemy->Multicast_StartGrowingDecal(WarningDecalMaterial, Anchor, ChargeDir, ChargeDistance, DecalWidth, 600.f, TelegraphTime);
        }
    }

    if (WindupMontage != nullptr)
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            ASC->PlayMontage(this, ActivationInfo, WindupMontage, 1.0f);
        }
    }

    // 텔레그래프 시간 후 돌진 시작
    World->GetTimerManager().SetTimer(PhaseTimer, this, &UGA_BossCharge::BeginCharge, FMath::Max(0.05f, TelegraphTime), false);
}

void UGA_BossCharge::BeginCharge()
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    UWorld* World = (Avatar != nullptr) ? Avatar->GetWorld() : nullptr;
    if (Avatar == nullptr || World == nullptr)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    // 텔레그래프 끝 — 데칼 제거
    if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Avatar))
    {
        Enemy->Multicast_DestroyWarningDecal();
    }

    // 돌진 중엔 폰 관통(벽은 막힘)
    if (UCapsuleComponent* Cap = Avatar->GetCapsuleComponent())
    {
        Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    }

    // 현재 위치 기준 돌진 구간 확정
    ChargeStartLoc = Avatar->GetActorLocation();
    ChargeEndLoc = ChargeStartLoc + ChargeDir * ChargeDistance;
    ChargeElapsed = 0.f;
    ChargeHitActors.Empty();

    if (ChargeMontage != nullptr)
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            ASC->PlayMontage(this, CurrentActivationInfo, ChargeMontage, 1.0f);
            // 돌진이 끝까지 가도록 첫 섹션을 자기 자신으로 루프(EndCharge에서 정지)
            const FName Sec = ChargeMontage->GetSectionName(0);
            if (Sec != NAME_None)
            {
                ASC->CurrentMontageSetNextSectionName(Sec, Sec);
            }
        }
    }

    // 16ms마다 전진 + 경로 타격
    World->GetTimerManager().SetTimer(ChargeTickTimer, this, &UGA_BossCharge::ChargeTick, 0.016f, true);
}

void UGA_BossCharge::ChargeTick()
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Avatar == nullptr || ChargeTime <= 0.f)
    {
        EndCharge();
        return;
    }

    ChargeElapsed += 0.016f;
    const float Alpha = FMath::Clamp(ChargeElapsed / ChargeTime, 0.f, 1.f);

    FVector NewLoc = FMath::Lerp(ChargeStartLoc, ChargeEndLoc, Alpha);
    NewLoc.Z = Avatar->GetActorLocation().Z;   // 수직은 그대로(지면 유지)
    Avatar->SetActorLocation(NewLoc, true);    // sweep: 벽 충돌 시 멈춤

    // 경로상 타격(같은 대상 1회)
    ApplyMeleeDamage(ChargeDamage, FGameplayTag(), 0.f, ChargeHitFeel, &ChargeHitActors);

    if (Alpha >= 1.f)
    {
        EndCharge();
    }
}

void UGA_BossCharge::EndCharge()
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    UWorld* World = (Avatar != nullptr) ? Avatar->GetWorld() : nullptr;
    if (World == nullptr)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    World->GetTimerManager().ClearTimer(ChargeTickTimer);

    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        if (EndMontage != nullptr)
        {
            ASC->PlayMontage(this, CurrentActivationInfo, EndMontage, 1.0f);
        }
        else
        {
            // 루프 중인 돌진 몽타주 정지 → 로코모션 복귀
            ASC->CurrentMontageStop();
        }
    }

    // 회복 시간 후 어빌리티 종료
    World->GetTimerManager().SetTimer(PhaseTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }), FMath::Max(0.05f, RecoverTime), false);
}

void UGA_BossCharge::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PhaseTimer);
        World->GetTimerManager().ClearTimer(ChargeTickTimer);
    }
    // 폰 충돌 복원
    if (ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (UCapsuleComponent* Cap = Avatar->GetCapsuleComponent())
        {
            Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        }
    }
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
