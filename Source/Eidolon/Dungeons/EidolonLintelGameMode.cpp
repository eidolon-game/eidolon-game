#include "EidolonLintelGameMode.h"
#include "EidolonLintelMantis.h"

FEidolonAbyssEncounter AEidolonLintelGameMode::GetEncounter() const
{
    return {
        3, TEXT("abyss_003_lintel_mantis_01"), TEXT("abyss_003_lintel_gate"),
        AEidolonLintelMantis::StaticClass(), 60.0, true,
        TEXT("Lintel Gallery | Abyss, floor 3"), TEXT("Galleria degli architravi | Abisso, livello 3"),
        TEXT("Leave the marked landing zone before the dive."), TEXT("Esci dal punto di atterraggio prima della picchiata.")
    };
}
