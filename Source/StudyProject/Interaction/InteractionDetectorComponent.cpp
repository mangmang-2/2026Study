#include "InteractionDetectorComponent.h"
#include "Interaction/InteractableInterface.h"
#include "DrawDebugHelpers.h"

UInteractionDetectorComponent::UInteractionDetectorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SphereRadius = 250.f;  // SetSphereRadius()는 UpdateBodySetup() → NewObject 호출 → 생성자에서 fatal
    SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void UInteractionDetectorComponent::BeginPlay()
{
    Super::BeginPlay();
    OnComponentBeginOverlap.AddDynamic(this, &UInteractionDetectorComponent::OnSphereBeginOverlap);
    OnComponentEndOverlap.AddDynamic(this, &UInteractionDetectorComponent::OnSphereEndOverlap);
}

void UInteractionDetectorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 감지 범위 시각화
    DrawDebugSphere(GetWorld(), GetComponentLocation(), SphereRadius, 16,
        Candidates.Num() > 0 ? FColor::Green : FColor::Red, false, -1.f, 0, 1.f);

    if (Candidates.Num() > 0)
    {
        UpdateFocus();
    }
}

void UInteractionDetectorComponent::OnSphereBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (OtherActor == nullptr) return;
    bool bImplements = OtherActor->Implements<UInteractable>();
    UE_LOG(LogTemp, Warning, TEXT("[Interaction] BeginOverlap: %s | IInteractable=%s"),
        *OtherActor->GetName(), bImplements ? TEXT("YES") : TEXT("NO"));
    if (bImplements)
    {
        Candidates.AddUnique(OtherActor);
    }
}

void UInteractionDetectorComponent::OnSphereEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, int32)
{
    Candidates.RemoveAll([OtherActor](const TWeakObjectPtr<AActor>& W) {
        return !W.IsValid() || W.Get() == OtherActor;
    });

    if (FocusedActor.Get() == OtherActor)
    {
        FocusedActor = nullptr;
        OnFocusChanged.Broadcast(nullptr);
    }
}

void UInteractionDetectorComponent::UpdateFocus()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    AActor* Best      = nullptr;
    float   BestDistSq = FLT_MAX;

    for (int32 i = Candidates.Num() - 1; i >= 0; --i)
    {
        if (!Candidates[i].IsValid()) { Candidates.RemoveAt(i); continue; }

        AActor* Candidate = Candidates[i].Get();
        if (!IInteractable::Execute_CanInteract(Candidate, Owner)) continue;  // 인터페이스 체크

        float DistSq = FVector::DistSquared(Owner->GetActorLocation(), Candidate->GetActorLocation());
        if (DistSq < BestDistSq) { BestDistSq = DistSq; Best = Candidate; }
    }

    if (Best != FocusedActor.Get())
    {
        FocusedActor = Best;
        OnFocusChanged.Broadcast(Best);
    }
}

void UInteractionDetectorComponent::TryInteract()
{
    UE_LOG(LogTemp, Warning, TEXT("[Interaction] TryInteract — FocusedActor: %s"),
        FocusedActor.IsValid() ? *FocusedActor->GetName() : TEXT("NULL"));
    if (FocusedActor.IsValid() == false) return;
    AActor* Target = FocusedActor.Get();
    if (Target->Implements<UInteractable>())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Interaction] Calling Execute_Interact on %s"), *Target->GetName());
        IInteractable::Execute_Interact(Target, GetOwner());
    }
}
