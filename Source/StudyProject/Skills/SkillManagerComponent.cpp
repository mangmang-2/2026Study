#include "SkillManagerComponent.h"
#include "SkillDefinition.h"
#include "GA_SkillExecutor.h"
#include "Character/PlayerCharacter.h"
#include "Combat/LockOnComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/HUD/SkillCastBarWorldWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

USkillManagerComponent::USkillManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void USkillManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    CooldownEndTime.Init(0.f, NumSlots);
    CooldownDuration.Init(0.f, NumSlots);

    if (EquippedSkills.Num() < NumSlots)
    {
        EquippedSkills.SetNum(NumSlots);
    }

    if (GetOwnerRole() == ROLE_Authority)
    {
        for (int32 i = 0; i < NumSlots; ++i)
        {
            if (DefaultSkills.IsValidIndex(i))
            {
                EquippedSkills[i] = DefaultSkills[i];
            }
        }
        EnsureExecutorGranted();
    }

    // 머리 위 캐스트바 위젯 컴포넌트(전 클라 — 타 플레이어 시전 표시)
    if (AActor* Owner = GetOwner())
    {
        CastBarWorldComp = NewObject<UWidgetComponent>(Owner, TEXT("CastBarWorldComp"));
        if (CastBarWorldComp != nullptr)
        {
            CastBarWorldComp->SetupAttachment(Owner->GetRootComponent());
            CastBarWorldComp->RegisterComponent();
            CastBarWorldComp->SetWidgetSpace(EWidgetSpace::Screen);
            CastBarWorldComp->SetDrawSize(FVector2D(150.f, 14.f));
            CastBarWorldComp->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
            CastBarWorldComp->SetVisibility(true);

            TSubclassOf<USkillCastBarWorldWidget> WClass = WorldCastBarWidgetClass
                ? WorldCastBarWidgetClass
                : TSubclassOf<USkillCastBarWorldWidget>(USkillCastBarWorldWidget::StaticClass());
            if (USkillCastBarWorldWidget* W = CreateWidget<USkillCastBarWorldWidget>(GetWorld(), WClass))
            {
                W->InitWorldCastBar(this);
                CastBarWorldComp->SetWidget(W);
            }
        }
    }

    OnSlotsChanged.Broadcast();
}

void USkillManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(USkillManagerComponent, EquippedSkills);
}

UAbilitySystemComponent* USkillManagerComponent::GetOwnerASC() const
{
    return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

void USkillManagerComponent::EnsureExecutorGranted()
{
    if (bExecutorGranted || GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    UAbilitySystemComponent* ASC = GetOwnerASC();
    if (ASC == nullptr)
    {
        return;
    }

    TSubclassOf<UGameplayAbility> Class = ExecutorAbilityClass
        ? TSubclassOf<UGameplayAbility>(ExecutorAbilityClass)
        : TSubclassOf<UGameplayAbility>(UGA_SkillExecutor::StaticClass());

    FGameplayAbilitySpec Spec(Class, 1, INDEX_NONE, GetOwner());
    ExecutorHandle = ASC->GiveAbility(Spec);
    bExecutorGranted = true;
}

void USkillManagerComponent::ActivateSlot(int32 SlotIndex)
{
    USkillDefinition* Skill = GetSlotSkill(SlotIndex);
    if (Skill == nullptr)
    {
        return;
    }

    if (GetCooldownRemaining(SlotIndex) > 0.f)
    {
        return;
    }

    // PointTarget 스킬은 홀드 동안 바닥 범위 데칼로 조준 → 릴리스 시 발동
    if (Skill->TargetingMode == ESkillTargetingMode::PointTarget)
    {
        BeginTargeting(SlotIndex);
        return;
    }

    FireSlot(SlotIndex);
}

void USkillManagerComponent::ReleaseSlot(int32 SlotIndex)
{
    if (bTargeting && TargetingSlot == SlotIndex)
    {
        EndTargeting(true);
    }
}

void USkillManagerComponent::FireSlot(int32 SlotIndex)
{
    USkillDefinition* Skill = GetSlotSkill(SlotIndex);
    if (Skill == nullptr || GetCooldownRemaining(SlotIndex) > 0.f)
    {
        return;
    }

    FVector Origin = FVector::ZeroVector;
    FVector Direction = FVector::ForwardVector;
    if (ResolveTargeting(Skill, Origin, Direction) == false)
    {
        return;
    }

    // 시전바 로컬 예측 — CastTime 동안 진행률 표시
    if (Skill->CastTime > 0.f && GetWorld() != nullptr)
    {
        CastingSkill = Skill;
        CastStartTime = GetWorld()->GetTimeSeconds();
        CastEndTime = CastStartTime + Skill->CastTime;
    }

    // 쿨다운은 서버가 발동 성공 시 시작한다(Server_ActivateSlot). 여기서 미리 켜면
    // 스탠드얼론/리슨서버에선 같은 인스턴스라 서버 쿨다운 체크가 자기 자신을 막아버림.
    Server_ActivateSlot(SlotIndex, Origin, Direction);
}

void USkillManagerComponent::BeginTargeting(int32 SlotIndex)
{
    bTargeting = true;
    TargetingSlot = SlotIndex;

    // 조준 동안 마우스 커서 표시(마법진 조준)
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (Pawn != nullptr)
    {
        if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
        {
            PC->SetShowMouseCursor(true);
            PC->SetInputMode(FInputModeGameAndUI());
        }
    }
}

void USkillManagerComponent::EndTargeting(bool bFire)
{
    const int32 Slot = TargetingSlot;

    // 발동을 커서가 살아있는 동안 먼저 한다. 커서를 먼저 숨기면 조준점이
    // 카메라 정면(캐릭터 발밑)으로 폴백돼 이펙트가 캐릭터에 뜬다.
    if (bFire)
    {
        FireSlot(Slot);
    }

    bTargeting = false;
    TargetingSlot = -1;

    if (PreviewDecal != nullptr)
    {
        PreviewDecal->DestroyComponent();
        PreviewDecal = nullptr;
    }

    APawn* Pawn = Cast<APawn>(GetOwner());
    if (Pawn != nullptr)
    {
        if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
        {
            PC->SetShowMouseCursor(false);
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
}

void USkillManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bTargeting == false)
    {
        return;
    }

    USkillDefinition* Skill = GetSlotSkill(TargetingSlot);
    if (Skill == nullptr)
    {
        EndTargeting(false);
        return;
    }

    FVector Point;
    if (GetGroundAimPoint(Point) == false)
    {
        return;
    }

    // 사거리 제한(수평)
    if (APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        FVector Flat = Point - Pawn->GetActorLocation();
        const float ZDiff = Flat.Z;
        Flat.Z = 0.f;
        if (Flat.Size() > Skill->Range)
        {
            Point = Pawn->GetActorLocation() + Flat.GetSafeNormal() * Skill->Range + FVector(0.f, 0.f, ZDiff);
        }
    }

    // 프리뷰 데칼 생성/갱신(로컬 코스메틱)
    if (PreviewDecal == nullptr)
    {
        UMaterialInterface* Mat = Skill->RangeDecalMaterial != nullptr ? Skill->RangeDecalMaterial : DefaultRangeDecalMaterial;
        PreviewDecal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), Mat, FVector(Skill->Radius), Point, FRotator(-90.f, 0.f, 0.f));
    }
    if (PreviewDecal != nullptr)
    {
        PreviewDecal->SetWorldLocation(Point);
        PreviewDecal->DecalSize = FVector(FMath::Max(64.f, Skill->Radius), Skill->Radius, Skill->Radius);
        PreviewDecal->MarkRenderStateDirty();
    }
}

void USkillManagerComponent::Server_ActivateSlot_Implementation(int32 SlotIndex, FVector Origin, FVector Direction)
{
    if (EquippedSkills.IsValidIndex(SlotIndex) == false)
    {
        return;
    }

    USkillDefinition* Skill = EquippedSkills[SlotIndex];
    if (Skill == nullptr)
    {
        return;
    }

    if (GetCooldownRemaining(SlotIndex) > 0.f)
    {
        return;
    }

    EnsureExecutorGranted();

    UAbilitySystemComponent* ASC = GetOwnerASC();
    if (ASC == nullptr || ExecutorHandle.IsValid() == false)
    {
        return;
    }

    PendingSkill = Skill;
    PendingOrigin = Origin;
    PendingDirection = Direction;

    if (ASC->TryActivateAbility(ExecutorHandle))
    {
        // 서버 권위 쿨다운(서버 게이팅 + 리슨호스트 UI) + 원격 소유 클라 UI 예측
        StartCooldown(SlotIndex, Skill->Cooldown);
        Client_StartCooldown(SlotIndex, Skill->Cooldown);

        // 머리 위 캐스트바 — 전 클라에 시전 시작 알림
        if (Skill->CastTime > 0.f)
        {
            Multicast_CastStarted(Skill->CastTime);
        }
    }
    else
    {
        PendingSkill = nullptr;
    }
}

void USkillManagerComponent::Client_StartCooldown_Implementation(int32 SlotIndex, float Duration)
{
    // 리슨호스트의 자기 폰에선 서버가 이미 StartCooldown 함 → 같은 값 재설정이라 무해
    StartCooldown(SlotIndex, Duration);
}

USkillDefinition* USkillManagerComponent::ConsumePendingActivation(FVector& OutOrigin, FVector& OutDirection)
{
    OutOrigin = PendingOrigin;
    OutDirection = PendingDirection;

    USkillDefinition* Skill = PendingSkill;
    PendingSkill = nullptr;
    return Skill;
}

void USkillManagerComponent::AssignSkill(USkillDefinition* Skill, int32 SlotIndex)
{
    if (SlotIndex < 0 || SlotIndex >= NumSlots)
    {
        return;
    }

    int32 PoolIndex = DefaultSkills.IndexOfByKey(Skill);
    Server_AssignSkill(PoolIndex, SlotIndex);
}

void USkillManagerComponent::Server_AssignSkill_Implementation(int32 PoolIndex, int32 SlotIndex)
{
    if (SlotIndex < 0 || SlotIndex >= NumSlots)
    {
        return;
    }

    if (EquippedSkills.Num() < NumSlots)
    {
        EquippedSkills.SetNum(NumSlots);
    }

    EquippedSkills[SlotIndex] = DefaultSkills.IsValidIndex(PoolIndex) ? DefaultSkills[PoolIndex].Get() : nullptr;

    // 서버(리슨)는 OnRep이 안 오므로 직접 갱신 통지
    OnSlotsChanged.Broadcast();
}

void USkillManagerComponent::OnRep_EquippedSkills()
{
    OnSlotsChanged.Broadcast();
}

void USkillManagerComponent::StartCooldown(int32 SlotIndex, float Duration)
{
    if (SlotIndex < 0 || SlotIndex >= NumSlots || GetWorld() == nullptr)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    CooldownEndTime[SlotIndex] = Now + Duration;
    CooldownDuration[SlotIndex] = Duration;

    OnCooldownStarted.Broadcast(SlotIndex, Duration);
}

float USkillManagerComponent::GetCooldownRemaining(int32 SlotIndex) const
{
    if (CooldownEndTime.IsValidIndex(SlotIndex) == false || GetWorld() == nullptr)
    {
        return 0.f;
    }

    return FMath::Max(0.f, CooldownEndTime[SlotIndex] - GetWorld()->GetTimeSeconds());
}

float USkillManagerComponent::GetCooldownFraction(int32 SlotIndex) const
{
    if (CooldownDuration.IsValidIndex(SlotIndex) == false || CooldownDuration[SlotIndex] <= 0.f)
    {
        return 0.f;
    }

    return FMath::Clamp(GetCooldownRemaining(SlotIndex) / CooldownDuration[SlotIndex], 0.f, 1.f);
}

USkillDefinition* USkillManagerComponent::GetSlotSkill(int32 SlotIndex) const
{
    return EquippedSkills.IsValidIndex(SlotIndex) ? EquippedSkills[SlotIndex] : nullptr;
}

TArray<USkillDefinition*> USkillManagerComponent::GetSkillPool() const
{
    TArray<USkillDefinition*> Out;
    Out.Reserve(DefaultSkills.Num());
    for (const TObjectPtr<USkillDefinition>& S : DefaultSkills)
    {
        Out.Add(S.Get());
    }
    return Out;
}

bool USkillManagerComponent::IsCasting() const
{
    return CastingSkill != nullptr && GetWorld() != nullptr && GetWorld()->GetTimeSeconds() < CastEndTime;
}

float USkillManagerComponent::GetCastProgress() const
{
    if (IsCasting() == false || CastEndTime <= CastStartTime)
    {
        return 0.f;
    }

    return FMath::Clamp((GetWorld()->GetTimeSeconds() - CastStartTime) / (CastEndTime - CastStartTime), 0.f, 1.f);
}

USkillDefinition* USkillManagerComponent::GetCastingSkill() const
{
    return IsCasting() ? CastingSkill : nullptr;
}

void USkillManagerComponent::Multicast_CastStarted_Implementation(float Duration)
{
    if (GetWorld() == nullptr)
    {
        return;
    }
    WorldCastStart = GetWorld()->GetTimeSeconds();
    WorldCastEnd = WorldCastStart + Duration;
}

bool USkillManagerComponent::IsWorldCasting() const
{
    return GetWorld() != nullptr && GetWorld()->GetTimeSeconds() < WorldCastEnd;
}

float USkillManagerComponent::GetWorldCastProgress() const
{
    if (IsWorldCasting() == false || WorldCastEnd <= WorldCastStart)
    {
        return 0.f;
    }
    return FMath::Clamp((GetWorld()->GetTimeSeconds() - WorldCastStart) / (WorldCastEnd - WorldCastStart), 0.f, 1.f);
}

bool USkillManagerComponent::ResolveTargeting(USkillDefinition* Skill, FVector& OutOrigin, FVector& OutDirection) const
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (Pawn == nullptr || Skill == nullptr)
    {
        return false;
    }

    const FVector PawnLoc = Pawn->GetActorLocation();
    APlayerController* PC = Cast<APlayerController>(Pawn->GetController());

    switch (Skill->TargetingMode)
    {
    case ESkillTargetingMode::NoTarget:
    {
        OutOrigin = PawnLoc;
        OutDirection = Pawn->GetActorForwardVector().GetSafeNormal2D();
        return true;
    }

    case ESkillTargetingMode::ActorTarget:
    {
        if (APlayerCharacter* Player = Cast<APlayerCharacter>(Pawn))
        {
            if (ULockOnComponent* Lock = Player->GetLockOnComponent())
            {
                if (Lock->IsLockedOn())
                {
                    if (AActor* Target = Lock->GetCurrentTarget())
                    {
                        OutOrigin = Target->GetActorLocation();
                        OutDirection = (OutOrigin - PawnLoc).GetSafeNormal2D();
                        return true;
                    }
                }
            }
        }
        // 록온 없으면 정면 방향으로 폴백
        OutDirection = (PC != nullptr ? PC->GetControlRotation().Vector() : Pawn->GetActorForwardVector()).GetSafeNormal2D();
        OutOrigin = PawnLoc + OutDirection * Skill->Range;
        return true;
    }

    case ESkillTargetingMode::DirectionTarget:
    {
        OutDirection = (PC != nullptr ? PC->GetControlRotation().Vector() : Pawn->GetActorForwardVector()).GetSafeNormal2D();
        OutOrigin = PawnLoc + OutDirection * Skill->Range;
        return true;
    }

    case ESkillTargetingMode::PointTarget:
    default:
    {
        FVector Point;
        if (GetGroundAimPoint(Point) == false)
        {
            Point = PawnLoc + Pawn->GetActorForwardVector().GetSafeNormal2D() * Skill->Range;
        }

        // 사거리 제한(수평)
        FVector Flat = Point - PawnLoc;
        const float ZDiff = Flat.Z;
        Flat.Z = 0.f;
        if (Flat.Size() > Skill->Range)
        {
            Point = PawnLoc + Flat.GetSafeNormal() * Skill->Range + FVector(0.f, 0.f, ZDiff);
        }

        OutOrigin = Point;
        OutDirection = (Point - PawnLoc).GetSafeNormal2D();
        return true;
    }
    }
}

bool USkillManagerComponent::GetGroundAimPoint(FVector& OutPoint) const
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (PC == nullptr || Pawn == nullptr || GetWorld() == nullptr)
    {
        return false;
    }

    // 커서(없으면 카메라) 광선 — 캐릭터/지오메트리에 안 걸리도록 평면 교차로 계산
    FVector RayOrigin = FVector::ZeroVector;
    FVector RayDir = FVector::ForwardVector;
    bool bGotRay = false;

    if (PC->bShowMouseCursor)
    {
        bGotRay = PC->DeprojectMousePositionToWorld(RayOrigin, RayDir);
    }
    if (bGotRay == false && PC->PlayerCameraManager != nullptr)
    {
        RayOrigin = PC->PlayerCameraManager->GetCameraLocation();
        RayDir = PC->PlayerCameraManager->GetCameraRotation().Vector();
        bGotRay = true;
    }
    if (bGotRay == false || FMath::Abs(RayDir.Z) < KINDA_SMALL_NUMBER)
    {
        return false;
    }

    // 캐스터 발밑 높이의 수평 평면과 광선의 교차점(폰/지오메트리 무시)
    const float PlaneZ = Pawn->GetActorLocation().Z;
    const float T = (PlaneZ - RayOrigin.Z) / RayDir.Z;
    if (T <= 0.f)
    {
        return false;
    }
    OutPoint = RayOrigin + RayDir * T;

    // 평면 지점에서 아래로 살짝 트레이스해 실제 지면 높이에 스냅(폰 제외)
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Pawn);
    FHitResult GroundHit;
    const FVector Up = OutPoint + FVector(0.f, 0.f, 300.f);
    const FVector Down = OutPoint - FVector(0.f, 0.f, 1000.f);
    if (GetWorld()->LineTraceSingleByChannel(GroundHit, Up, Down, ECC_WorldStatic, Params))
    {
        OutPoint.Z = GroundHit.ImpactPoint.Z;
    }

    return true;
}
