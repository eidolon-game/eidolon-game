#include "EidolonSuspendedTrunksGameMode.h"

#include "../EidolonCharacter.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonForestStalker.h"
#include "EidolonSuspendedBridgeRules.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"

FEidolonAbyssEncounter AEidolonSuspendedTrunksGameMode::GetEncounter() const
{
    using namespace EidolonAbyss;
    FEidolonAbyssEncounter Floor;
    Floor.Floor = SuspendedTrunksFloor;
    Floor.GateTag = TEXT("abyss_017_gate");
    Floor.TitleEnglish = TEXT("Suspended Trunks | Abyss, floor 17");
    Floor.TitleItalian = TEXT("Tronchi sospesi | Abisso, livello 17");
    Floor.FightEnglish = TEXT("Reach the island and align the bridge with the far bank.");
    Floor.FightItalian = TEXT("Raggiungi l'isola e allinea la passerella alla sponda lontana.");
    Floor.CompletionKind = EventKind::SuspendedBridgeAligned;
    Floor.CompletionTarget = Target::SuspendedBridgeControl;
    Floor.CompletionControlTag = TEXT("abyss_017_bridge_control");
    return Floor;
}

bool AEidolonSuspendedTrunksGameMode::BindCompletionActors(UWorld& World)
{
    using EidolonAbyss::FindTaggedActor;
    Bridge = FindTaggedActor<AStaticMeshActor>(World, TEXT("abyss_017_bridge"));
    const auto* OpenPose = FindTaggedActor<ATargetPoint>(World, TEXT("abyss_017_bridge_open_pose"));
    if (!Bridge.IsValid() || !OpenPose ||
        Bridge->GetStaticMeshComponent()->Mobility != EComponentMobility::Movable ||
        !Bridge->GetStaticMeshComponent()->GetStaticMesh())
    {
        return false;
    }

    Pivot = Bridge->GetActorLocation();
    RestPose = Bridge->GetActorRotation();
    AlignedPose = OpenPose->GetActorRotation();
    const auto Delta = EidolonSuspendedBridge::QuarterTurnDelta(RestPose.Yaw, AlignedPose.Yaw);
    if (!Delta || !OpenPose->GetActorLocation().Equals(Pivot, 1.0) ||
        !FMath::IsNearlyZero(RestPose.Pitch) || !FMath::IsNearlyZero(RestPose.Roll) ||
        !FMath::IsNearlyZero(AlignedPose.Pitch) || !FMath::IsNearlyZero(AlignedPose.Roll))
    {
        return false;
    }
    AlignmentDelta = *Delta;
    return true;
}

bool AEidolonSuspendedTrunksGameMode::IsRotationClear(const AStaticMeshActor& Deck) const
{
    const auto* Mesh = Deck.GetStaticMeshComponent()->GetStaticMesh();
    if (!Mesh) return false;

    constexpr int32 Samples = 32;
    constexpr double ClearanceAboveDeckCm = 95.0;
    constexpr double RailWidthCm = 25.0;
    const FBoxSphereBounds Bounds = Mesh->GetBounds();
    const FVector CenterOffset = Bounds.Origin * Deck.GetActorScale3D();
    const FVector Extent = Bounds.BoxExtent * Deck.GetActorScale3D().GetAbs();
    // Expand each sample enough to cover the angular gap to its neighbors.
    const double MaxRadius = (Extent + CenterOffset.GetAbs() + FVector(0.0, RailWidthCm, 0.0)).Size2D();
    const double Padding = 2.0 * MaxRadius * FMath::Sin(FMath::DegreesToRadians(FMath::Abs(AlignmentDelta) / (4.0 * Samples)));
    const FCollisionShape SweptDeck = FCollisionShape::MakeBox(
        Extent + FVector(Padding, Padding + RailWidthCm, ClearanceAboveDeckCm));
    FCollisionObjectQueryParams Objects(ECC_Pawn);
    Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonSuspendedBridge), false, &Deck);
    TArray<AActor*> DeckParts;
    Deck.GetAttachedActors(DeckParts, true, true);
    Query.AddIgnoredActors(DeckParts);

    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FRotator Pose(0.0, RestPose.Yaw + AlignmentDelta * (static_cast<double>(Index) / Samples), 0.0);
        const FVector Center = Pivot + Pose.RotateVector(CenterOffset) + FVector(0.0, 0.0, ClearanceAboveDeckCm);
        if (GetWorld()->OverlapAnyTestByObjectType(Center, Pose.Quaternion(), Objects, SweptDeck, Query))
            return false;
    }
    return true;
}

bool AEidolonSuspendedTrunksGameMode::InteractWithControl(AEidolonCharacter& Player, const ATargetPoint& Control)
{
    const auto* Deck = Bridge.Get();
    if (!Deck || !CanInteractAt(Player, &Control) ||
        !Deck->GetActorLocation().Equals(Pivot, 1.0) || !Deck->GetActorRotation().Equals(RestPose, 1.0))
    {
        return false;
    }
    bBridgeOccupied = !IsRotationClear(*Deck);
    return !bBridgeOccupied;
}

void AEidolonSuspendedTrunksGameMode::UpdateCompletionActors(bool bCompleted)
{
    if (auto* Deck = Bridge.Get())
        Deck->SetActorRotation(bCompleted ? AlignedPose : RestPose, ETeleportType::TeleportPhysics);
}

FString AEidolonSuspendedTrunksGameMode::GetControlPrompt() const
{
    if (!bBridgeOccupied) return Super::GetControlPrompt();
    return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
        ? TEXT("Aspetta che la passerella e il suo percorso siano liberi.")
        : TEXT("Wait until the bridge and its path are clear.");
}

void AEidolonSuspendedTrunksGameMode::SpawnOptionalThreats()
{
    auto* World = GetWorld();
    if (!World) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    const FName Tags[]{TEXT("abyss_017_stalker_west"), TEXT("abyss_017_stalker_east")};
    for (const FName Tag : Tags)
    {
        if (const auto* Spawn = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, Tag))
        {
            World->SpawnActor<AEidolonForestStalker>(Spawn->GetActorLocation() + FVector(0, 0, 95.0),
                Spawn->GetActorRotation(), Params);
        }
    }
}
