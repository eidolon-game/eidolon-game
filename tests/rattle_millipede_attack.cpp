#include "../Source/Eidolon/Dungeons/EidolonRattleMillipedeRules.h"

#include <iostream>
#include <limits>

using namespace EidolonRattleMillipede;

int main()
{
    if (AdjacentWaypoint(0, 4, true) != 1 || AdjacentWaypoint(3, 4, true) != 0
        || AdjacentWaypoint(0, 4, false) != 3 || AdjacentWaypoint(2, 4, false) != 1
        || AdjacentWaypoint(0, 0, true) != 0 || AdjacentWaypoint(7, 4, true) != 7)
    {
        std::cerr << "Patrol did not follow or reverse its ordered waypoints\n";
        return 1;
    }

    for (const int Side : {-1, 1})
    {
        Attack Whip;
        if (Whip.Begin(false, 0.0, Side * 100.0, 60.0) || Whip.Begin(true, NoseCm + 1.0, Side * 100.0, 60.0)
            || Whip.Begin(true, 0.0, 0.0, 60.0) || !Whip.Begin(true, 0.0, Side * 100.0, 60.0)
            || Whip.LockedSide != Side || Whip.Begin(true, 0.0, -Side * 100.0, 60.0)
            || Whip.TryHit(true, 0.0, Side * 100.0, 60.0))
        {
            std::cerr << "Side lock or harmless telegraph failed\n";
            return 1;
        }

        Whip.AdvanceTime(TelegraphSeconds);
        if (Whip.Current != Phase::Strike || Whip.TryHit(false, 0.0, Side * 100.0, 60.0)
            || Whip.TryHit(true, 0.0, -Side * 100.0, 60.0)
            || Whip.TryHit(true, TailCm - 1.0, Side * 100.0, 60.0)
            || Whip.TryHit(true, 0.0, Side * (OuterSideCm + 1.0), 60.0)
            || Whip.TryHit(true, 0.0, Side * 100.0, HeightCm + 1.0)
            || Whip.TryHit(true, 0.0, std::numeric_limits<double>::quiet_NaN(), 0.0)
            || !Whip.TryHit(true, TailCm, Side * OuterSideCm, HeightCm)
            || Whip.TryHit(true, 0.0, Side * 120.0, 60.0))
        {
            std::cerr << "Evade, occlusion, locked side or single-hit receipt failed\n";
            return 1;
        }

        Whip.AdvanceTime(StrikeSeconds);
        if (Whip.Current != Phase::Recovery || Whip.Begin(true, 0.0, Side * 100.0, 60.0))
        {
            std::cerr << "Strike did not enter protected recovery\n";
            return 1;
        }
        Whip.AdvanceTime(RecoverySeconds);
        if (Whip.Current != Phase::Ready || Whip.LockedSide != 0
            || !Whip.Begin(true, 0.0, -Side * 100.0, 60.0))
        {
            std::cerr << "Later strike could not choose the opposite flank\n";
            return 1;
        }
    }

    std::cout << "Rattle Millipede: waypoint reversal, either flank, evade and single-hit recovery passed\n";
}
