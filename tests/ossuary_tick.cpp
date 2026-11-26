#include "../Source/Eidolon/Dungeons/EidolonOssuaryTickRules.h"

#include <cassert>
#include <iostream>
#include <limits>

int main()
{
    using namespace EidolonOssuaryTick;

    Attack State;
    assert(!State.Begin(false, 10000.0));
    assert(!State.Begin(true, AggroRange * AggroRange + 1.0));
    assert(!State.Begin(true, std::numeric_limits<double>::quiet_NaN()));
    assert(State.Begin(true, AggroRange * AggroRange));
    assert(!State.Begin(true, 10000.0));
    assert(State.LeapDistance(1.0) == 0.0);
    State.AdvanceTime(WindupSeconds / 2);
    assert(State.Current == Phase::Windup);
    State.AdvanceTime(WindupSeconds / 2);
    assert(State.Current == Phase::Leap);
    assert(State.LeapDistance(10.0) == LeapSpeed * LeapSeconds);
    assert(State.Land(true));
    assert(!State.Land(true));
    assert(State.Remaining == HitRecoverySeconds);
    State.AdvanceTime(HitRecoverySeconds);
    assert(State.Current == Phase::Ready);

    assert(State.Begin(true, 10000.0));
    State.AdvanceTime(WindupSeconds);
    State.AdvanceTime(LeapSeconds);
    assert(State.Current == Phase::Recovery);
    assert(State.Remaining == MissRecoverySeconds);
    assert(MissRecoverySeconds > HitRecoverySeconds);
    assert(!State.Begin(true, 10000.0));
    State.AdvanceTime(MissRecoverySeconds);
    assert(State.Current == Phase::Ready);

    std::cout << "Ossuary Tick telegraph, single impact, and miss recovery passed\n";
}
