#include "EidolonObliqueClearingGameMode.h"

FEidolonAbyssEncounter AEidolonObliqueClearingGameMode::GetEncounter() const
{
    using namespace EidolonAbyss;
    FEidolonAbyssEncounter Floor;
    Floor.Floor = ObliqueClearingFloor;
    Floor.TitleEnglish = TEXT("Oblique Clearing | Abyss, floor 12");
    Floor.TitleItalian = TEXT("Radura obliqua | Abisso, livello 12");
    Floor.FightEnglish = TEXT("Follow the trail to the stairs. The side path leads to the overlook.");
    Floor.FightItalian = TEXT("Segui il sentiero fino alla scala. Il sentiero laterale porta al belvedere.");
    Floor.CompletionKind = EventKind::FloorTraversed;
    Floor.CompletionTarget = Target::TraversalExit;
    return Floor;
}
