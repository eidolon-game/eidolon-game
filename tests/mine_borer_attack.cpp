#include "../Source/Eidolon/Dungeons/EidolonMineBorerRules.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace EidolonMineBorer;

int main()
{
    Attack Borer;
    const double NaN = std::numeric_limits<double>::quiet_NaN();
    assert(!Borer.Begin(false, 500, 0, 0));
    assert(!Borer.Begin(true, 500, HazardHalfWidthCm + 1, 0));
    assert(!Borer.Begin(true, 500, 0, 151));
    assert(!Borer.Begin(true, NaN, 0, 0));

    assert(Borer.Begin(true, 640, 0, 0));
    assert(Borer.Current == Phase::Approach);
    assert(Borer.ChargeDistance(.5) == 0);
    Borer.Advance(ApproachSeconds);
    assert(Borer.Current == Phase::Windup);
    assert(!Borer.Begin(true, 640, 0, 0));
    assert(Borer.ChargeDistance(0.5) == 0);
    Borer.Advance(0.89);
    assert(Borer.Current == Phase::Windup);
    Borer.Advance(0.02);
    assert(Borer.Current == Phase::Charge);
    assert(Borer.ChargeDistance(9.0) == ChargeLimitCm);
    assert(Borer.ChargeDistance(NaN) == 0);

    Borer.Stop();
    assert(Borer.Current == Phase::Impact);
    assert(Borer.DebrisCanHit(350, HazardHalfWidthCm, 0));
    assert(!Borer.DebrisCanHit(350, HazardHalfWidthCm + 1, 0));
    assert(!Borer.DebrisCanHit(900, 0, 0));
    assert(!Borer.DebrisCanHit(-350, 0, 0));
    assert(Borer.DamageMultiplier(-1.0) == 0.80);

    Borer.Advance(ImpactSeconds);
    assert(Borer.Current == Phase::Recovery);
    assert(!Borer.DebrisCanHit(350, 0, 0));
    assert(Borer.DamageMultiplier(-1.0) > 1.0);
    assert(Borer.DamageMultiplier(0.0) == 0.80);
    assert(Borer.DamageMultiplier(1.0) == 0.80);
    Borer.Advance(RecoverySeconds);
    assert(Borer.Current == Phase::Ready);

    assert(Borer.Begin(true, 500, 0, 0));
    Borer.Advance(ApproachSeconds);
    Borer.Advance(WindupSeconds);
    Borer.Advance(ChargeSeconds);
    assert(Borer.Current == Phase::Impact);
    std::cout << "Mine Borer: fixed charge, protected flanks, bounded debris and rear recovery passed\n";
}
