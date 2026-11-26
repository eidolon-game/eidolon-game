#pragma once
#include "CoreMinimal.h"

// Pure policy. Callers must supply authoritative Save/Quest state.
// Chapter numbers alone never grant a seal or a fast-travel destination.
namespace EidolonWorld
{
enum class ERegion : uint8 { Valdris, Giants, Continent, Vareth, Oblivion };
struct FAccessSnapshot
{
    int32 Chapter = 0;
    bool bAegisSeal = false;
    bool bHorizonSeal = false;
    bool bVarethErased = false;
    bool bOblivionAccess = false;
    bool bFreedomEnding = false;
};
inline bool CanEnter(ERegion Region, const FAccessSnapshot& State)
{
    if (State.Chapter < 1 || State.Chapter > 26) return false;
    const bool Giants = State.bFreedomEnding || (State.Chapter >= 5 && State.bAegisSeal);
    const bool Continent = State.bFreedomEnding || (Giants && State.Chapter >= 8 && State.bHorizonSeal);
    switch (Region)
    {
        case ERegion::Valdris: return true;
        case ERegion::Giants: return Giants;
        case ERegion::Continent: return Continent;
        case ERegion::Vareth: return Continent && State.Chapter >= 10 && !State.bVarethErased;
        case ERegion::Oblivion: return State.Chapter >= 20 && State.bOblivionAccess;
        default: return false;
    }
}
inline bool CanFastTravel(ERegion Region, const FAccessSnapshot& State,
                          bool bDiscovered, bool bSafeArrival, bool bInCombat)
{
    return bDiscovered && bSafeArrival && !bInCombat && CanEnter(Region, State);
}
}
