#include "../Source/Eidolon/Dungeons/EidolonVaultPincerRules.h"

#include <cassert>
#include <limits>

int main()
{
    using namespace EidolonVaultPincer;
    Attack Pincer;
    assert(Pincer.DamageMultiplier(1.0) == GuardedDamageMultiplier);
    assert(Pincer.DamageMultiplier(0.0) == 1.0f);
    assert(Pincer.DamageMultiplier(-1.0) == 1.0f);
    assert(!Pincer.Begin(100.0, 0.0, false));
    assert(!Pincer.Begin(ReachCm + 1.0, 0.0, true));
    assert(!Pincer.Begin(100.0, HeightCm + 1.0, true));
    assert(!Pincer.Begin(std::numeric_limits<double>::quiet_NaN(), 0.0, true));

    assert(Pincer.Begin(ReachCm, HeightCm, true));
    assert(Pincer.DamageMultiplier(1.0) == GuardedDamageMultiplier);
    assert(!Pincer.Hits(100.0, 0.0, 1.0, true));
    assert(!Pincer.Advance(TelegraphSeconds * 0.5));
    assert(Pincer.Advance(TelegraphSeconds));
    assert(Pincer.Hits(ReachCm, HeightCm, ArcMinimumDot, true));
    assert(!Pincer.Hits(100.0, 0.0, ArcMinimumDot - 0.01, true));
    assert(!Pincer.Hits(ReachCm + 1.0, 0.0, 1.0, true));
    assert(!Pincer.Hits(100.0, -HeightCm - 1.0, 1.0, true));
    assert(!Pincer.Hits(100.0, 0.0, 1.0, false));
    assert(!Pincer.Advance(SweepSeconds * 0.5));

    assert(!Pincer.Advance(SweepSeconds));
    assert(!Pincer.Hits(100.0, 0.0, 1.0, true));
    assert(Pincer.DamageMultiplier(1.0) == 1.0f);
    assert(!Pincer.Begin(100.0, 0.0, true));
    assert(!Pincer.Advance(RecoverySeconds * 0.5));
    assert(Pincer.DamageMultiplier(1.0) == 1.0f);
    assert(!Pincer.Advance(RecoverySeconds));
    assert(Pincer.DamageMultiplier(1.0) == GuardedDamageMultiplier);
    assert(Pincer.Begin(100.0, 0.0, true));
}
