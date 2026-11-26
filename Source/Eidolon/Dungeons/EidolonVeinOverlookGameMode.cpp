#include "EidolonVeinOverlookGameMode.h"

FEidolonAbyssEncounter AEidolonVeinOverlookGameMode::GetEncounter() const
{
    using namespace EidolonAbyss;
    FEidolonAbyssEncounter Floor;
    Floor.Floor = VeinOverlookFloor;
    Floor.TitleEnglish = TEXT("Vein Overlook | Abyss, floor 19");
    Floor.TitleItalian = TEXT("Belvedere della vena | Abisso, livello 19");
    Floor.FightEnglish = TEXT("Look out over the fissure or follow the stone ramp down.");
    Floor.FightItalian = TEXT("Osserva la faglia dal belvedere oppure scendi per la rampa di pietra.");
    Floor.CompletionKind = EventKind::FloorTraversed;
    Floor.CompletionTarget = Target::TraversalExit;
    return Floor;
}
