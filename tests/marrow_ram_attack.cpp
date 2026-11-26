#include "../Source/Eidolon/Dungeons/EidolonMarrowRamRules.h"

#include <iostream>
#include <limits>

using namespace EidolonMarrowRam;

int main()
{
    Attack Blocked;
    if (Blocked.Begin(false, 400.0) || Blocked.Begin(true, MaxLockRangeCm + 1.0)
        || Blocked.Begin(true, std::numeric_limits<double>::quiet_NaN())
        || !Blocked.Begin(true, 400.0) || Blocked.Begin(true, 200.0)
        || Blocked.Stop(Impact::Obstacle) || Blocked.ChargeDistance(1.0) != 0.0)
    {
        std::cerr << "Invalid targets or collision before the two-scrape lock were accepted\n";
        return 1;
    }

    Blocked.AdvanceTime(FirstScrapeSeconds);
    if (Blocked.Current != Phase::SecondScrape || Blocked.ChargeDistance(1.0) != 0.0)
    {
        std::cerr << "Charge skipped the second scrape\n";
        return 1;
    }
    Blocked.AdvanceTime(SecondScrapeSeconds);
    const double Part = Blocked.ChargeDistance(ChargeSeconds / 4.0);
    Blocked.AdvanceTime(ChargeSeconds / 4.0);
    const double Rest = Blocked.ChargeDistance(ChargeSeconds * 10.0);
    if (Blocked.Current != Phase::Charge || std::abs(Part + Rest - ChargeLengthCm) > 1e-9
        || Blocked.ChargeDistance(-1.0) != 0.0 || Blocked.ChargeDistance(std::numeric_limits<double>::infinity()) != 0.0
        || Blocked.Stop(static_cast<Impact>(99)))
    {
        std::cerr << "Sweep did not preserve bounded remaining travel\n";
        return 1;
    }

    if (!Blocked.Stop(Impact::Obstacle) || Blocked.Stop(Impact::Obstacle)
        || Blocked.Current != Phase::Recovery || Blocked.Remaining != ObstacleRecoverySeconds
        || Blocked.DamageMultiplier() != ObstacleVulnerability || Blocked.ChargeDistance(1.0) != 0.0)
    {
        std::cerr << "Obstacle did not stop and expose the Ram exactly once\n";
        return 1;
    }
    Blocked.AdvanceTime(FreeRecoverySeconds);
    if (Blocked.Current != Phase::Recovery)
    {
        std::cerr << "Obstacle recovery ended like a free brake\n";
        return 1;
    }
    Blocked.AdvanceTime(ObstacleRecoverySeconds);
    if (Blocked.Current != Phase::Ready || Blocked.DamageMultiplier() != 1.0) return 1;

    Attack Hit;
    if (!Hit.Begin(true, MinLockRangeCm)) return 1;
    Hit.AdvanceTime(FirstScrapeSeconds);
    Hit.AdvanceTime(SecondScrapeSeconds);
    if (!Hit.Stop(Impact::Player) || Hit.LastImpact != Impact::Player
        || Hit.Remaining != PlayerRecoverySeconds || Hit.DamageMultiplier() != 1.0)
    {
        std::cerr << "Player impact did not enter its own recovery\n";
        return 1;
    }

    Attack Free;
    if (!Free.Begin(true, MaxLockRangeCm)) return 1;
    Free.AdvanceTime(FirstScrapeSeconds);
    Free.AdvanceTime(SecondScrapeSeconds);
    Free.AdvanceTime(ChargeSeconds);
    if (Free.LastImpact != Impact::FreeBrake || Free.Remaining != FreeRecoverySeconds
        || Free.DamageMultiplier() != 1.0 || Free.Stop(Impact::Player))
    {
        std::cerr << "Unobstructed charge did not brake without a second impact\n";
        return 1;
    }

    std::cout << "Marrow Ram: two scrapes, bounded sweep and distinct obstacle, player and free recovery passed\n";
}
