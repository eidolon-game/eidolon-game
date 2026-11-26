#include "EidolonFirstDescentInteraction.h"

#include "CollisionQueryParams.h"
#include "EidolonInteractionRange.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

bool FEidolonFirstDescentInteraction::BindTarget(int32 StepIndex, std::string_view TargetId, AActor& Actor)
{
    if (!Actor.HasAuthority() || Actor.IsActorBeingDestroyed() || !Actor.GetWorld()
        || StepIndex < 0 || static_cast<std::size_t>(StepIndex) >= EidolonFirstDescent::Steps.size()
        || TargetId != EidolonFirstDescent::Steps[StepIndex].Target)
    {
        return false;
    }

    if (const TWeakObjectPtr<AActor>* Existing = Targets.Find(StepIndex))
    {
        if (const AActor* ExistingActor = Existing->Get())
        {
            if (!ExistingActor->IsActorBeingDestroyed())
                return ExistingActor == &Actor;
        }
    }

    Targets.Add(StepIndex, TWeakObjectPtr<AActor>(&Actor));
    return true;
}

bool FEidolonFirstDescentInteraction::CanInteract(const APawn& Player, const AActor& RequestedTarget, int32 Cursor,
                                                  const EidolonFirstDescent::Event& ProposedEvent, double MaxDistanceCm) const
{
    if (Cursor < 0 || !EidolonFirstDescent::Advance(static_cast<std::size_t>(Cursor), ProposedEvent))
        return false;

    return CanReachTarget(Player, RequestedTarget, Cursor, MaxDistanceCm);
}

bool FEidolonFirstDescentInteraction::CanReachTarget(const APawn& Player, const AActor& RequestedTarget,
                                                     int32 StepIndex, double MaxDistanceCm) const
{
    const TWeakObjectPtr<AActor>* Registered = Targets.Find(StepIndex);
    UWorld* World = Player.GetWorld();
    if (!Registered || Registered->Get() != &RequestedTarget || !World || World != RequestedTarget.GetWorld()
        || !Player.HasAuthority() || !Player.IsPlayerControlled() || Player.IsActorBeingDestroyed()
        || RequestedTarget.IsActorBeingDestroyed())
    {
        return false;
    }

    const FVector TargetLocation = RequestedTarget.GetActorLocation();
    if (!EidolonQuest::IsWithinInteractionRange(FVector::DistSquared(Player.GetActorLocation(), TargetLocation), MaxDistanceCm))
        return false;

    FVector Eyes;
    FRotator EyeRotation;
    Player.GetActorEyesViewPoint(Eyes, EyeRotation);
    if (Eyes.ContainsNaN()) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EidolonFirstDescentInteraction), false, &Player);
    FHitResult Hit;
    return !World->LineTraceSingleByChannel(Hit, Eyes, TargetLocation, ECC_Visibility, QueryParams)
        || Hit.GetActor() == &RequestedTarget;
}
