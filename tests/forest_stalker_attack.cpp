#include "../Source/Eidolon/Dungeons/EidolonForestStalkerRules.h"

#include <iostream>
#include <limits>

using namespace EidolonForestStalker;

int main()
{
    Search AwarenessState;
    AwarenessState.Observe(false, 1.0);
    if (AwarenessState.Current != Awareness::Returning) return 1;
    AwarenessState.Observe(true, 1.0);
    if (AwarenessState.Current != Awareness::Tracking || AwarenessState.Remaining != SearchSeconds) return 1;
    AwarenessState.Observe(false, 0.7);
    if (AwarenessState.Current != Awareness::Searching) return 1;
    const double BeforeInvalidTime = AwarenessState.Remaining;
    AwarenessState.Observe(false, -1.0);
    if (AwarenessState.Remaining != BeforeInvalidTime) return 1;
    AwarenessState.Observe(true, 0.1);
    AwarenessState.Observe(false, SearchSeconds);
    if (AwarenessState.Current != Awareness::Returning || AwarenessState.Remaining != 0.0)
    {
        std::cerr << "Search did not return after its last sighting expired\n";
        return 1;
    }

    Attack Sweep;
    if (Sweep.Begin(false, 200.0) || Sweep.Begin(true, PounceRangeCm + 1.0)
        || !Sweep.Begin(true, SweepRangeCm) || Sweep.Action != Move::RootSweep
        || Sweep.TrySweepHit(true, 150.0, 30.0, 10.0))
    {
        std::cerr << "Unseen target, range or harmless sweep windup failed\n";
        return 1;
    }
    Sweep.AdvanceTime(SweepWindupSeconds);
    if (Sweep.TrySweepHit(false, 150.0, 30.0, 10.0) || Sweep.TrySweepHit(true, -20.0, 0.0, 0.0)
        || Sweep.TrySweepHit(true, 10.0, 150.0, 0.0) || Sweep.TrySweepHit(true, 160.0, 0.0, MaxHeightDeltaCm + 1.0)
        || !Sweep.TrySweepHit(true, 180.0, 30.0, 20.0) || Sweep.TrySweepHit(true, 180.0, 30.0, 20.0))
    {
        std::cerr << "Flanking, occlusion, height or single-hit sweep failed\n";
        return 1;
    }
    Sweep.AdvanceTime(SweepActiveSeconds);
    if (Sweep.Current != Phase::Recovery || Sweep.Remaining != HitRecoverySeconds || Sweep.DamageMultiplier() != 1.0)
        return 1;

    Attack Pounce;
    if (!Pounce.Begin(true, PounceRangeCm) || Pounce.Action != Move::Pounce
        || Pounce.Begin(true, 300.0) || Pounce.PounceDistance(1.0) != 0.0 || Pounce.StopPounce(true))
        return 1;
    Pounce.AdvanceTime(PounceWindupSeconds);
    const double FirstPart = Pounce.PounceDistance(PounceActiveSeconds / 2.0);
    Pounce.AdvanceTime(PounceActiveSeconds / 2.0);
    if (std::abs(FirstPart + Pounce.PounceDistance(1.0) - PounceRangeCm) > 1e-9
        || Pounce.PounceDistance(std::numeric_limits<double>::quiet_NaN()) != 0.0
        || !Pounce.StopPounce(false) || Pounce.StopPounce(true)
        || Pounce.Remaining != MissRecoverySeconds || Pounce.DamageMultiplier() <= 1.0)
    {
        std::cerr << "Pounce did not keep locked travel or expose joints after missing\n";
        return 1;
    }
    Pounce.AdvanceTime(MissRecoverySeconds);
    if (!Pounce.Begin(true, 300.0)) return 1;
    Pounce.AdvanceTime(PounceWindupSeconds);
    if (!Pounce.StopPounce(true) || Pounce.Remaining != HitRecoverySeconds)
        return 1;

    std::cout << "Forest Stalker: sight memory, search expiry, evadable sweep, locked pounce and recovery passed\n";
}
