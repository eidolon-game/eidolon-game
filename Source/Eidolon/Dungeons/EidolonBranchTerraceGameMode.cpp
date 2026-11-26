#include "EidolonBranchTerraceGameMode.h"

#include "EidolonAbyssActorBindings.h"
#include "EidolonForestStalker.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"

FEidolonAbyssEncounter AEidolonBranchTerraceGameMode::GetEncounter() const
{
    using namespace EidolonAbyss;
    FEidolonAbyssEncounter Floor;
    Floor.Floor = BranchTerraceFloor;
    Floor.GateTag = TEXT("abyss_013_gate");
    Floor.TitleEnglish = TEXT("Branch Terraces | Abyss, floor 13");
    Floor.TitleItalian = TEXT("Terrazze dei rami | Abisso, livello 13");
    Floor.FightEnglish = TEXT("Take either route, then lower the ladder from the upper terrace.");
    Floor.FightItalian = TEXT("Scegli un percorso e abbassa la scala dalla terrazza alta.");
    Floor.CompletionKind = EventKind::BranchStairReleased;
    Floor.CompletionTarget = Target::BranchStairRelease;
    Floor.CompletionControlTag = TEXT("abyss_013_stair_release");
    Floor.ShortcutBlockTag = TEXT("abyss_013_ladder_block");
    return Floor;
}

void AEidolonBranchTerraceGameMode::SpawnOptionalThreats()
{
    using EidolonAbyss::FindTaggedActor;
    auto* World = GetWorld();
    if (!World) return;

    const auto* Spawn = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_013_stalker_spawn"));
    if (!Spawn) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    auto* Stalker = World->SpawnActor<AEidolonForestStalker>(Spawn->GetActorLocation() + FVector(0, 0, 95.0),
        Spawn->GetActorRotation(), Params);
    if (!Stalker) return;

    const auto* Perch = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_013_stalker_perch"));
    const auto* Landing = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_013_stalker_landing"));
    if (Perch && Landing) Stalker->ConfigurePerch(Perch->GetActorLocation(), Landing->GetActorLocation());
}
