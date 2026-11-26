#pragma once

#include <algorithm>
#include <cmath>

namespace EidolonMarrowRam
{
    enum class Phase { Ready, FirstScrape, SecondScrape, Charge, Recovery };
    enum class Impact { FreeBrake, Player, Obstacle };

    inline constexpr double FirstScrapeSeconds = 0.60;
    inline constexpr double SecondScrapeSeconds = 0.55;
    inline constexpr double ChargeSeconds = 0.75;
    inline constexpr double ChargeLengthCm = 900.0;
    inline constexpr double MinLockRangeCm = 24.0;
    inline constexpr double MaxLockRangeCm = 650.0;
    inline constexpr double ObstacleRecoverySeconds = 2.0;
    inline constexpr double PlayerRecoverySeconds = 1.15;
    inline constexpr double FreeRecoverySeconds = 0.85;
    inline constexpr double ObstacleVulnerability = 1.5;

    struct Attack
    {
        Phase Current = Phase::Ready;
        Impact LastImpact = Impact::FreeBrake;
        double Remaining = 0.0;

        bool Begin(bool Visible, double DistanceCm)
        {
            if (Current != Phase::Ready || !Visible || !std::isfinite(DistanceCm)
                || DistanceCm < MinLockRangeCm || DistanceCm > MaxLockRangeCm)
            {
                return false;
            }
            Current = Phase::FirstScrape;
            Remaining = FirstScrapeSeconds;
            return true;
        }

        // The caller sweeps this full movement with the direction captured by Begin.
        double ChargeDistance(double DeltaSeconds) const
        {
            if (Current != Phase::Charge || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
                return 0.0;
            return ChargeLengthCm * (std::min(DeltaSeconds, Remaining) / ChargeSeconds);
        }

        bool Stop(Impact Result)
        {
            if (Current != Phase::Charge) return false;

            switch (Result)
            {
                case Impact::FreeBrake: Remaining = FreeRecoverySeconds; break;
                case Impact::Player: Remaining = PlayerRecoverySeconds; break;
                case Impact::Obstacle: Remaining = ObstacleRecoverySeconds; break;
                default: return false;
            }
            LastImpact = Result;
            Current = Phase::Recovery;
            return true;
        }

        void AdvanceTime(double DeltaSeconds)
        {
            if (Current == Phase::Ready || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
                return;

            Remaining = std::max(0.0, Remaining - DeltaSeconds);
            if (Remaining > 0.0) return;

            switch (Current)
            {
                case Phase::FirstScrape:
                    Current = Phase::SecondScrape;
                    Remaining = SecondScrapeSeconds;
                    break;
                case Phase::SecondScrape:
                    Current = Phase::Charge;
                    Remaining = ChargeSeconds;
                    break;
                case Phase::Charge:
                    Stop(Impact::FreeBrake);
                    break;
                case Phase::Recovery:
                    Current = Phase::Ready;
                    break;
                case Phase::Ready:
                    break;
            }
        }

        double DamageMultiplier() const
        {
            return Current == Phase::Recovery && LastImpact == Impact::Obstacle
                ? ObstacleVulnerability : 1.0;
        }
    };
}
