#pragma once

#include <algorithm>
#include <cmath>

namespace EidolonOssuaryTick
{
    enum class Phase { Ready, Windup, Leap, Recovery };

    inline constexpr double WindupSeconds = 0.65;
    inline constexpr double LeapSeconds = 0.32;
    inline constexpr double HitRecoverySeconds = 0.65;
    inline constexpr double MissRecoverySeconds = 1.35;
    inline constexpr double LeapSpeed = 980.0;
    inline constexpr double AggroRange = 400.0;

    struct Attack
    {
        Phase Current = Phase::Ready;
        double Remaining = 0.0;

        bool Begin(bool TargetVisible, double DistanceSquared)
        {
            if (Current != Phase::Ready || !TargetVisible || !std::isfinite(DistanceSquared) ||
                DistanceSquared <= 0.0 || DistanceSquared > AggroRange * AggroRange)
            {
                return false;
            }
            Current = Phase::Windup;
            Remaining = WindupSeconds;
            return true;
        }

        // The caller sweeps the entire returned movement before advancing time.
        double LeapDistance(double DeltaSeconds) const
        {
            if (Current != Phase::Leap || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
                return 0.0;
            return LeapSpeed * std::min(Remaining, DeltaSeconds);
        }

        bool Land(bool HitPlayer)
        {
            if (Current != Phase::Leap) return false;
            Current = Phase::Recovery;
            Remaining = HitPlayer ? HitRecoverySeconds : MissRecoverySeconds;
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
                    Current = Phase::Leap;
                    Remaining = LeapSeconds;
                    break;
                case Phase::Leap:
                    Land(false);
                    break;
                case Phase::Recovery:
                    Current = Phase::Ready;
                    break;
                case Phase::Ready:
                    break;
            }
        }
    };
}
