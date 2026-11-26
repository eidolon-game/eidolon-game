#pragma once

namespace EidolonWorld
{
    enum class Region
    {
        Valdris,
        GiantTerritory,
        Continent,
        Vareth,
        Oblivion,
        Stair,
        Empyrea
    };

    struct Progress
    {
        int Chapter = 0;
        int GiantsDefeated = 0;
        int StairFloor = 0;
        bool AegisSeal = false;
        bool HorizonSeal = false;
    };

    inline bool CanEnter(Region Destination, const Progress& State)
    {
        if (State.Chapter < 1 || State.Chapter > 26) return false;

        const bool OuterValdris = State.Chapter >= 5 && State.AegisSeal;
        const bool Aetheryon = OuterValdris && State.Chapter >= 8 &&
            State.HorizonSeal && State.GiantsDefeated >= 100;

        switch (Destination)
        {
            case Region::Valdris: return true;
            case Region::GiantTerritory: return OuterValdris;
            case Region::Continent: return Aetheryon;
            case Region::Vareth: return Aetheryon && State.Chapter == 10;
            case Region::Oblivion: return Aetheryon && State.Chapter >= 20;
            case Region::Stair: return Aetheryon && State.Chapter >= 23;
            case Region::Empyrea: return Aetheryon && State.Chapter >= 24 && State.StairFloor == 100;
        }
        return false;
    }

    inline bool CanFastTravel(Region Destination, const Progress& State,
                              bool Discovered, bool SafeArrival, bool InCombat)
    {
        return Discovered && SafeArrival && !InCombat && CanEnter(Destination, State);
    }
}
