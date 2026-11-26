#include "EidolonOssuaryGameMode.h"
#include "EidolonOssuaryTick.h"

FEidolonAbyssEncounter AEidolonOssuaryGameMode::GetEncounter() const
{
    return {
        2, TEXT("abyss_002_ossuary_tick_01"), TEXT("abyss_002_ossuary_gate"),
        AEidolonOssuaryTick::StaticClass(), 30.0, false,
        TEXT("Low-Arch Ossuary | Abyss, floor 2"), TEXT("Ossario degli Archi Bassi | Abisso, livello 2"),
        TEXT("Evade the leap, then attack during recovery."), TEXT("Schiva il salto e attacca durante il recupero.")
    };
}
