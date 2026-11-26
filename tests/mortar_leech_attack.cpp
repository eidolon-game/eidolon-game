#include "../Source/Eidolon/Dungeons/EidolonMortarLeechRules.h"

#include <cassert>
#include <limits>

int main()
{
    using namespace EidolonMortarLeech;
    Attack Lance;
    assert(!Lance.Begin(200.0, 0.0, 150.0));
    assert(!Lance.Begin(MaxReachCm + 1.0, 0.0, 1000.0));
    assert(!Lance.Begin(200.0, HeightCm + 1.0, MaxReachCm));
    assert(!Lance.Begin(std::numeric_limits<double>::quiet_NaN(), 0.0, MaxReachCm));

    assert(Lance.Begin(200.0, 0.0, 300.0));
    assert(!Lance.Begin(100.0, 0.0, MaxReachCm));
    assert(!Lance.Contains(200.0, 0.0, 0.0));
    assert(!Lance.Advance(TelegraphSeconds * 0.5));
    assert(Lance.Advance(TelegraphSeconds));
    assert(Lance.Contains(300.0, HalfWidthCm, HeightCm));
    assert(!Lance.Contains(301.0, 0.0, 0.0));
    assert(!Lance.Contains(-1.0, 0.0, 0.0));
    assert(!Lance.Contains(200.0, HalfWidthCm + 1.0, 0.0));
    assert(!Lance.Contains(200.0, 0.0, HeightCm + 1.0));
    assert(!Lance.Advance(StrikeSeconds * 0.5));
    assert(!Lance.Advance(StrikeSeconds));
    assert(!Lance.Contains(200.0, 0.0, 0.0));
    assert(!Lance.Begin(200.0, 0.0, 300.0));
    assert(!Lance.Advance(RecoverySeconds));
    assert(Lance.Begin(200.0, 0.0, 1000.0));
    assert(Lance.LengthCm == MaxReachCm);
    assert(Lance.Advance(5.0));
    assert(Lance.Current == Phase::Strike);
}
