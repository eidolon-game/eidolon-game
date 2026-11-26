#include "EidolonBuriedForestEntryGameMode.h"
#include "EidolonForestStalker.h"

FEidolonAbyssEncounter AEidolonBuriedForestEntryGameMode::GetEncounter() const
{
    return {
        11, TEXT("abyss_011_forest_stalker_01"), TEXT("abyss_011_forest_gate"),
        AEidolonForestStalker::StaticClass(), 95.0, false,
        TEXT("Buried Forest Threshold | Abyss, floor 11"), TEXT("Soglia della Foresta Sepolta | Abisso, livello 11"),
        TEXT("Break sight behind the roots, then evade the pounce."),
        TEXT("Usa le radici come riparo, poi schiva il balzo.")
    };
}
