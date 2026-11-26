#pragma once

#include <algorithm>
#include <cmath>

namespace EidolonLintelMantis
{
    enum class Phase { Ready, Windup, Dive, HitRecovery, MissRecovery };

    inline constexpr double WindupSeconds = 0.95;
    inline constexpr double DiveSeconds = 0.42;
    inline constexpr double HitRecoverySeconds = 0.85;
    inline constexpr double MissRecoverySeconds = 2.10;
    inline constexpr double MinLockRangeCm = 20.0;
    inline constexpr double MaxLockRangeCm = 520.0;
    inline constexpr double OvershootCm = 80.0;
    inline constexpr double MissVulnerability = 1.6;

    inline double DropProgress(double Travel, double Clearance, double LockedDistance)
    {
        if (Clearance >= LockedDistance) return 0.0;
        return std::clamp((Travel - Clearance) / (LockedDistance - Clearance), 0.0, 1.0);
    }

    struct Attack
    {
        Phase Current = Phase::Ready;
        double Remaining = 0.0;
        double LockedDistanceCm = 0.0;

        bool Begin(bool Visible, double TargetDistanceCm)
        {
            if (Current != Phase::Ready || !Visible || !std::isfinite(TargetDistanceCm) ||
                TargetDistanceCm < MinLockRangeCm || TargetDistanceCm > MaxLockRangeCm)
            {
                return false;
            }
            LockedDistanceCm = TargetDistanceCm + OvershootCm;
            Current = Phase::Windup;
            Remaining = WindupSeconds;
            return true;
        }

        // Integrate the locked trajectory for the portion of this tick in Dive.
        double DiveDistance(double DeltaSeconds) const
        {
            if (Current != Phase::Dive || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
                return 0.0;
            return LockedDistanceCm * (std::min(DeltaSeconds, Remaining) / DiveSeconds);
        }

        bool Land(bool HitLockedPlayer)
        {
            if (Current != Phase::Dive) return false;
            Current = HitLockedPlayer ? Phase::HitRecovery : Phase::MissRecovery;
            Remaining = HitLockedPlayer ? HitRecoverySeconds : MissRecoverySeconds;
            return true;
        }

        void AdvanceTime(double DeltaSeconds)
        {
            if (Current == Phase::Ready || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0) return;
            Remaining = std::max(0.0, Remaining - DeltaSeconds);
            if (Remaining > 0.0) return;

            switch (Current)
            {
                case Phase::Windup:
                    Current = Phase::Dive;
                    Remaining = DiveSeconds;
                    break;
                case Phase::Dive:
                    Land(false);
                    break;
                case Phase::HitRecovery:
                case Phase::MissRecovery:
                    Current = Phase::Ready;
                    LockedDistanceCm = 0.0;
                    break;
                case Phase::Ready:
                    break;
            }
        }

        double DamageMultiplier() const
        {
            return Current == Phase::MissRecovery ? MissVulnerability : 1.0;
        }
    };
}
