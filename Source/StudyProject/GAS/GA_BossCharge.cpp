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
    // 적(서버 권위) 공격이므로 서버에서만 실행
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // 데미지 GE(C++ UGE_Damage) — BP 없이 C++로 부여
    DamageGEClass = UGE_Damage::StaticClass();

    // 돌진 중에는 AI/이동이 이 어빌리티에 종속
    bLocksMovement = true;

    // 돌진 동안 슈퍼아머 — 피격당해도(데미지는 들어감) 피격모션으로 끊기지 않음.
    // GA_HitReact가 Status.SuperArmor를 ActivationBlockedTags로 막아 플린치를 차단.
    ActivationOwnedTags.AddTag(StudyTags::Status_SuperArmor);

    // 돌진 활성 중(텔레그래프 포함) 보스를 정지 — AI 컨트롤러가 State.Attacking이면 이동/회전 안 함.
    // (텔레그래프 동안 보스가 움직이면 발밑에서 자라는 데칼과 어긋남)
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

    // 발동 순간 플레이어 방향으로 돌진 방향 고정(이후 플레이어가 움직여도 추적 안 함 → 회피 가능)
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

    // 보스를 돌진 방향으로 회전
    Avatar->SetActorRotation(FRotator(0.f, ChargeDir.Rotation().Yaw, 0.f));

    // 바닥 워닝 데칼 — 보스 발밑(Anchor)에서 끝점까지 텔레그래프 시간 동안 자란다(다 자라면 돌진).
    // 데칼은 코스메틱이라 서버 스폰만으론 다른 클라에 안 보임 → 보스 멀티캐스트로 전 클라가 표시.
    if (WarningDecalMaterial != nullptr)
    {
        if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Avatar))
        {
            const float HalfHeight = Avatar->GetSimpleCollisionHalfHeight();
            const FVector Anchor(Avatar->GetActorLocation().X, Avatar->GetActorLocation().Y,
                Avatar->GetActorLocation().Z - HalfHeight + 2.f);
            // 투영깊이 600(계단/경사 커버), 폭=DecalWidth, 길이=ChargeDistance, 성장시간=텔레그래프
            Enemy->Multicast_StartGrowingDecal(WarningDecalMaterial, Anchor, ChargeDir, ChargeDistance, DecalWidth, 600.f, TelegraphTime);
        }
    }

    // 윈드업 몽타주(있으면)
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

    // 텔레그래프(데칼 성장) 끝 → 워닝 데칼 제거(전 클라 동기)
    if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Avatar))
    {
        Enemy->Multicast_DestroyWarningDecal();
    }

    // 돌진 동안 폰 충돌 무시 — 플레이어/적에 막히지 않고 관통한다.
    // (벽 등 World 채널은 여전히 sweep으로 막히고, 데미지는 별도 트레이스라 관통 중에도 명중)
    if (UCapsuleComponent* Cap = Avatar->GetCapsuleComponent())
    {
        Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    }

    // 돌진 시작 지점/끝 지점 확정(텔레그래프 끝난 현재 위치 기준)
    ChargeStartLoc = Avatar->GetActorLocation();
    ChargeEndLoc = ChargeStartLoc + ChargeDir * ChargeDistance;
    ChargeElapsed = 0.f;
    ChargeHitActors.Empty();

    // 돌진 몽타주(루프)
    if (ChargeMontage != nullptr)
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            ASC->PlayMontage(this, CurrentActivationInfo, ChargeMontage, 1.0f);
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

    // 경로상 대상 타격(같은 대상 1회) — 베이스의 근접 데미지 재사용
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

    // 마무리 몽타주(있으면)
    if (EndMontage != nullptr)
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            ASC->PlayMontage(this, CurrentActivationInfo, EndMontage, 1.0f);
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
    // 폰 충돌 복원(돌진 종료/취소 모두 커버)
    if (ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (UCapsuleComponent* Cap = Avatar->GetCapsuleComponent())
        {
            Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        }
    }
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
