#include "EidolonFernBasinGameMode.h"

#include "EidolonAbyssActorBindings.h"
#include "EidolonAbyssProgression.h"
#include "EidolonForestStalker.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"

FEidolonDrainLayout AEidolonFernBasinGameMode::GetDrainLayout() const
{
    return {EidolonAbyss::FernBasinFloor, TEXT("abyss_014_drain_upper"), TEXT("abyss_014_drain_lower"),
        TEXT("abyss_014_gate"), 0.0, -20.0, false,
        TEXT("Fern Basin | Abyss, floor 14"), TEXT("Bacino delle felci | Abisso, livello 14"),
        TEXT("Open both drains."), TEXT("Apri le due prese di drenaggio."),
        TEXT("One drain is open. Find the other one."), TEXT("Una presa è aperta. Cerca l'altra."),
        TEXT("The channel is clear. Use the lower stair."), TEXT("Il canale è libero. Usa la scala verso il basso."),
        TEXT("This drain is already open."), TEXT("Questa presa è già aperta.")};
}

AActor* AEidolonFernBasinGameMode::SpawnThreat(UWorld& World)
{
    const auto* Anchor = EidolonAbyss::FindTaggedActor<ATargetPoint>(World, TEXT("abyss_014_stalker_spawn"));
    if (!Anchor) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    return World.SpawnActor<AEidolonForestStalker>(Anchor->GetActorLocation() + FVector(0, 0, 95.0),
        Anchor->GetActorRotation(), Params);
}
