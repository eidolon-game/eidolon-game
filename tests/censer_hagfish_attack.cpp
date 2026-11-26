#include "../Source/Eidolon/Dungeons/EidolonCenserHagfishRules.h"

#include <iostream>
#include <limits>

using namespace EidolonCenserHagfish;

int main()
{
    Cast First;
    Cast Second;
    if (First.Begin(false, 200.0, 450.0) || First.Begin(true, MaxLockRangeCm + 1.0, 620.0)
        || First.Begin(true, 200.0, 199.0) || !First.Begin(true, 300.0, 420.0)
        || !First.HoldsWorldClaim() || Second.Begin(!First.HoldsWorldClaim(), 200.0, 450.0)
        || First.LengthCm != 420.0 || First.TryHit(200.0, 0.0, 74.0))
    {
        std::cerr << "Cast claim, wall clipping or harmless telegraph failed\n";
        return 1;
    }

    First.AdvanceTime(TelegraphSeconds / 2.0);
    if (First.Current != Phase::Telegraph || First.TryHit(200.0, 0.0, 74.0))
    {
        std::cerr << "The strip became active before the telegraph finished\n";
        return 1;
    }

    First.AdvanceTime(TelegraphSeconds);
    if (First.Current != Phase::Active || Second.Begin(!First.HoldsWorldClaim(), 200.0, 450.0)
        || First.TryHit(-1.0, 0.0, 74.0) || First.TryHit(421.0, 0.0, 74.0)
        || First.TryHit(200.0, HalfWidthCm + 1.0, 74.0)
        || First.TryHit(200.0, -HalfWidthCm - 1.0, 74.0)
        || First.TryHit(200.0, 0.0, HeightToleranceCm + 1.0)
        || First.TryHit(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0)
        || !First.TryHit(420.0, HalfWidthCm, HeightToleranceCm)
        || First.TryHit(200.0, 0.0, 74.0))
    {
        std::cerr << "Active strip bounds, evasion, exclusive cast or single impact failed\n";
        return 1;
    }

    First.AdvanceTime(ActiveSeconds);
    if (First.Current != Phase::Recovery || First.HoldsWorldClaim()
        || !Second.Begin(!First.HoldsWorldClaim(), 250.0, 1000.0)
        || Second.LengthCm != MaxStripLengthCm || First.TryHit(150.0, 0.0, 74.0))
    {
        std::cerr << "Recovery did not release the cast or clamp strip length\n";
        return 1;
    }

    Second.AdvanceTime(TelegraphSeconds);
    Second.AdvanceTime(ActiveSeconds);
    First.AdvanceTime(RecoverySeconds);
    if (!First.Begin(!Second.HoldsWorldClaim(), 150.0, 420.0))
    {
        std::cerr << "A later cast could not claim the strip\n";
        return 1;
    }
    First.AdvanceTime(TelegraphSeconds);
    if (!First.TryHit(150.0, 0.0, 74.0))
    {
        std::cerr << "A new cast did not reset the single-impact receipt\n";
        return 1;
    }

    std::cout << "Censer Hagfish: telegraph, strip clipping, evasion, exclusivity and single impact passed\n";
}
