#include "EidolonLockOnComponent.h"
#include "EidolonTargetable.h"
#include "EidolonTargetingPolicy.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

UEidolonLockOnComponent::UEidolonLockOnComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}
AActor* UEidolonLockOnComponent::GetTarget() const { return Target.Get(); }

bool UEidolonLockOnComponent::IsEligible(AActor* Actor, FVector Origin, FVector& Point) const
{
    if (!IsValid(Actor) || Actor == GetOwner() || !Actor->GetClass()->ImplementsInterface(UEidolonTargetable::StaticClass()))
        return false;
    if (!IEidolonTargetable::Execute_CanBeTargeted(Actor)) return false;
    Point = IEidolonTargetable::Execute_GetTargetLocation(Actor);
    if (Point.ContainsNaN() || FVector::DistSquared(Origin, Point) > FMath::Square(Range)) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EidolonLockOn), false, GetOwner());
    return !GetWorld()->LineTraceSingleByChannel(Hit, Origin, Point, ECC_Visibility, Params) || Hit.GetActor() == Actor;
}
bool UEidolonLockOnComponent::Acquire()
{
    auto* Pawn = Cast<APawn>(GetOwner());
    auto* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || !PC->IsLocalController() || !GetWorld()) return false;
    FVector Origin; FRotator View;
    PC->GetPlayerViewPoint(Origin, View);
    TArray<FOverlapResult> Hits;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EidolonAcquire), false, Pawn);
    GetWorld()->OverlapMultiByObjectType(Hits, Origin, FQuat::Identity, Objects,
        FCollisionShape::MakeSphere(Range), Params);
    AActor* Best = nullptr;
    double BestScore = TNumericLimits<double>::Max();
    TSet<AActor*> Seen;
    for (const FOverlapResult& Hit : Hits)
    {
        AActor* Candidate = Hit.GetActor();
        if (Seen.Contains(Candidate)) continue;
        Seen.Add(Candidate);
        FVector Point;
        if (!IsEligible(Candidate, Origin, Point)) continue;
        const FVector Offset = Point - Origin;
        const double Score = Eidolon::Targeting::Score(Offset.Size(),
            FVector::DotProduct(View.Vector(), Offset.GetSafeNormal()), Range, MinAlignment);
        if (Score >= 0.0 && Score < BestScore) { Best = Candidate; BestScore = Score; }
    }
    if (!Best) { Release(); return false; }
    const bool bChanged = !bLocked || Target.Get() != Best;
    Target = Best;
    bLocked = true;
    SetComponentTickEnabled(true);
    if (bChanged) OnTargetChanged.Broadcast(Best);
    return true;
}
void UEidolonLockOnComponent::Release()
{
    const bool bWasLocked = bLocked;
    bLocked = false;
    Target.Reset();
    SetComponentTickEnabled(false);
    if (bWasLocked) OnTargetChanged.Broadcast(nullptr);
}
void UEidolonLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    auto* Pawn = Cast<APawn>(GetOwner());
    auto* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || !PC->IsLocalController() || !GetWorld()) { Release(); return; }
    FVector Origin; FRotator View;
    PC->GetPlayerViewPoint(Origin, View);
    FVector Point;
    if (!IsEligible(Target.Get(), Origin, Point)) { Release(); return; }
    FRotator Desired = (Point - Origin).Rotation();
    Desired.Roll = 0.f;
    PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(), Desired, DeltaTime, RotationSpeed));
}
void UEidolonLockOnComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Release();
    Super::EndPlay(Reason);
}
