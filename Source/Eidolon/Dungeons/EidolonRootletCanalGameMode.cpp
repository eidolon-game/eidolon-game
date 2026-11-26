#include "EidolonRootletCanalGameMode.h"

#include "../EidolonCharacter.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonForestStalker.h"
#include "EidolonRootletCanalRules.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"

FEidolonAbyssEncounter AEidolonRootletCanalGameMode::GetEncounter() const
{
    using namespace EidolonAbyss;
    FEidolonAbyssEncounter Floor;
    Floor.Floor = RootletCanalFloor;
    Floor.GateTag = TEXT("abyss_016_gate");
    Floor.TitleEnglish = TEXT("Rootlet Canal | Abyss, floor 16");
    Floor.TitleItalian = TEXT("Canale delle radichette | Abisso, livello 16");
    Floor.FightEnglish = TEXT("Follow the low canal to release the counterweight from behind the gate.");
    Floor.FightItalian = TEXT("Segui il canale basso e libera il contrappeso dal retro della saracinesca.");
    Floor.CompletionKind = EventKind::RootletCounterweightReleased;
    Floor.CompletionTarget = Target::RootletCounterweight;
    Floor.CompletionControlTag = TEXT("abyss_016_counterweight_release");
    return Floor;
}

bool AEidolonRootletCanalGameMode::BindCompletionActors(UWorld& World)
{
    using EidolonAbyss::FindTaggedActor;
    Portcullis = FindTaggedActor<AStaticMeshActor>(World, GetEncounter().GateTag);
    if (!Portcullis.IsValid()) return false;

    ClosedGatePosition = Portcullis->GetActorLocation();
    RaisedGate = FindTaggedActor<ATargetPoint>(World, TEXT("abyss_016_gate_raised"));
    Counterweight = FindTaggedActor<AStaticMeshActor>(World, TEXT("abyss_016_counterweight"));
    RestingWeight = FindTaggedActor<ATargetPoint>(World, TEXT("abyss_016_weight_released"));
    if (auto* Weight = Counterweight.Get())
    {
        HeldWeightPosition = Weight->GetActorLocation();
        Weight->SetActorEnableCollision(false);
    }
    return true;
}

void AEidolonRootletCanalGameMode::UpdateCompletionActors(bool bCompleted)
{
    if (auto* Gate = Portcullis.Get())
    {
        if (const auto* Raised = RaisedGate.Get())
        {
            const FVector Pose = bCompleted ? Raised->GetActorLocation() : ClosedGatePosition;
            if (Gate->SetActorLocation(Pose) || Gate->GetActorLocation().Equals(Pose))
                Gate->SetActorHiddenInGame(false);
        }
    }
    if (auto* Weight = Counterweight.Get())
    {
        if (const auto* Rest = RestingWeight.Get())
            Weight->SetActorLocation(bCompleted ? Rest->GetActorLocation() : HeldWeightPosition);
    }
}

bool AEidolonRootletCanalGameMode::InteractWithControl(AEidolonCharacter& Player, const ATargetPoint& Control)
{
    const auto* Gate = Portcullis.Get();
    if (!Gate) return false;
    const FVector Normal = Gate->GetActorForwardVector();
    return EidolonRootletCanal::IsOnReleaseSide(
        FVector::DotProduct(Player.GetActorLocation() - ClosedGatePosition, Normal),
        FVector::DotProduct(Control.GetActorLocation() - ClosedGatePosition, Normal))
        && CanInteractAt(Player, &Control);
}

void AEidolonRootletCanalGameMode::SpawnOptionalThreats()
{
    auto* World = GetWorld();
    if (!World) return;
    const auto* Anchor = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_016_stalker_spawn"));
    if (!Anchor) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    auto* Stalker = World->SpawnActor<AEidolonForestStalker>(Anchor->GetActorLocation() + FVector(0, 0, 95.0),
        Anchor->GetActorRotation(), Params);
    if (Stalker) Stalker->EnableCanalGroundFollowing();
}
