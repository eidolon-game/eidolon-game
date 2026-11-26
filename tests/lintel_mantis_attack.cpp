#include "../Source/Eidolon/Dungeons/EidolonLintelMantisRules.h"

#include <iostream>
#include <limits>

using namespace EidolonLintelMantis;

int main()
{
    if (DropProgress(160.0, 165.0, 300.0) != 0.0 ||
        DropProgress(232.5, 165.0, 300.0) != 0.5 ||
        DropProgress(320.0, 165.0, 300.0) != 1.0 ||
        DropProgress(200.0, 240.0, 200.0) != 0.0)
    {
        std::cerr << "The dive must clear the ledge before descending\n";
        return 1;
    }
    Attack Miss;
    if (Miss.Begin(false, 200.0) || Miss.Begin(true, MinLockRangeCm - 1.0)
        || Miss.Begin(true, MaxLockRangeCm + 1.0)
        || Miss.Begin(true, std::numeric_limits<double>::quiet_NaN())
        || !Miss.Begin(true, MaxLockRangeCm) || Miss.Begin(true, 150.0)
        || Miss.Land(true) || Miss.DiveDistance(1.0) != 0.0)
    {
        std::cerr << "Invalid target, lock, or windup attack accepted\n";
        return 1;
    }

    Miss.AdvanceTime(WindupSeconds / 2.0);
    if (Miss.Current != Phase::Windup || Miss.DamageMultiplier() != 1.0)
    {
        std::cerr << "Windup ended early or made the Mantis vulnerable\n";
        return 1;
    }
    Miss.AdvanceTime(WindupSeconds);
    const double HalfDistance = Miss.DiveDistance(DiveSeconds / 2.0);
    Miss.AdvanceTime(DiveSeconds / 2.0);
    const double RestDistance = Miss.DiveDistance(DiveSeconds * 10.0);
    if (std::abs(HalfDistance + RestDistance - (MaxLockRangeCm + OvershootCm)) > 1e-9
        || Miss.DiveDistance(-1.0) != 0.0)
    {
        std::cerr << "Dive did not respect the locked trajectory or remaining distance\n";
        return 1;
    }

    Miss.AdvanceTime(DiveSeconds);
    if (Miss.Current != Phase::MissRecovery || Miss.Remaining != MissRecoverySeconds
        || Miss.DamageMultiplier() != MissVulnerability || Miss.Land(true) || Miss.Begin(true, 150.0))
    {
        std::cerr << "Expired dive did not open the vulnerable miss recovery\n";
        return 1;
    }

    Miss.AdvanceTime(HitRecoverySeconds);
    if (Miss.Current != Phase::MissRecovery)
    {
        std::cerr << "Miss recovery ended at the shorter hit duration\n";
        return 1;
    }
    Miss.AdvanceTime(MissRecoverySeconds);
    if (Miss.Current != Phase::Ready || Miss.LockedDistanceCm != 0.0)
    {
        std::cerr << "Miss recovery did not reset\n";
        return 1;
    }

    Attack Blocked;
    if (!Blocked.Begin(true, 250.0)) return 1;
    Blocked.AdvanceTime(WindupSeconds);
    if (!Blocked.Land(false) || Blocked.Current != Phase::MissRecovery)
    {
        std::cerr << "A blocking obstacle did not count as a miss\n";
        return 1;
    }

    Attack Hit;
    if (!Hit.Begin(true, MinLockRangeCm)) return 1;
    Hit.AdvanceTime(WindupSeconds);
    if (!Hit.Land(true) || Hit.Land(true) || Hit.Current != Phase::HitRecovery
        || Hit.Remaining != HitRecoverySeconds || Hit.DamageMultiplier() != 1.0)
    {
        std::cerr << "Hit recovery or duplicate impact handling failed\n";
        return 1;
    }
    Hit.AdvanceTime(HitRecoverySeconds);
    if (Hit.Current != Phase::Ready) return 1;

    std::cout << "Lintel Mantis: target lock, swept distance, obstacle miss, hit and vulnerable miss recovery passed\n";
}
