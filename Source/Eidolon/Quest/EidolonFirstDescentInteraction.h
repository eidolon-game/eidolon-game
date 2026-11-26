#pragma once

#include "Containers/Map.h"
#include "EidolonFirstDescentRules.h"
#include "UObject/WeakObjectPtrTemplates.h"

class AActor;
class APawn;

// An authoritative scene adapter owns one registry for the lifetime of its world.
// Only that adapter should bind actors from trusted scene configuration.
class FEidolonFirstDescentInteraction
{
public:
    bool BindTarget(int32 StepIndex, std::string_view TargetId, AActor& Actor);

    // Physical interaction precondition. The producer must still confirm the
    // actual gameplay outcome before submitting a campaign event.
    bool CanInteract(const APawn& Player, const AActor& RequestedTarget, int32 Cursor,
                     const EidolonFirstDescent::Event& ProposedEvent, double MaxDistanceCm) const;

    // Validate a physical gate without proposing progress. Travel events are
    // committed by the destination adapter after successful arrival.
    bool CanReachTarget(const APawn& Player, const AActor& RequestedTarget, int32 StepIndex,
                        double MaxDistanceCm) const;

private:
    TMap<int32, TWeakObjectPtr<AActor>> Targets;
};
