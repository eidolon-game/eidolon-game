#pragma once

#include <algorithm>
#include <cmath>

namespace EidolonVaultPincer
{
    enum class Phase { Guard, Telegraph, Sweep, Recovery };

    inline constexpr double ReachCm = 225.0;
    inline constexpr double HeightCm = 105.0;
    inline constexpr double ArcHalfAngleDegrees = 58.0;
    inline constexpr double ArcMinimumDot = 0.5299192642332049;
    inline constexpr double FrontGuardMinimumDot = 0.5;
    inline constexpr double TelegraphSeconds = 0.8;
    inline constexpr double SweepSeconds = 0.25;
    inline constexpr double RecoverySeconds = 1.55;
    inline constexpr float GuardedDamageMultiplier = 0.30f;

    struct Attack
    {
        Phase Current = Phase::Guard;
        double Remaining = 0.0;

        bool Begin(double Distance, double Height, bool Visible)
        {
            if (Current != Phase::Guard || !Visible || !InRange(Distance, Height))
                return false;

            Current = Phase::Telegraph;
            Remaining = TelegraphSeconds;
            return true;
        }

        bool Advance(double DeltaSeconds)
        {
            if (Current == Phase::Guard || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
                return false;

            Remaining = std::max(0.0, Remaining - DeltaSeconds);
            if (Remaining > 0.0) return false;

            if (Current == Phase::Telegraph)
            {
                Current = Phase::Sweep;
                Remaining = SweepSeconds;
                return true;
            }
            if (Current == Phase::Sweep)
            {
                Current = Phase::Recovery;
                Remaining = RecoverySeconds;
            }
            else
            {
                Current = Phase::Guard;
            }
            return false;
        }

        bool Hits(double Distance, double Height, double FacingDot, bool Visible) const
        {
            return Current == Phase::Sweep && Visible && InRange(Distance, Height)
                && FacingDot >= ArcMinimumDot;
        }

        float DamageMultiplier(double FacingDot) const
        {
            return Current != Phase::Recovery && FacingDot >= FrontGuardMinimumDot
                ? GuardedDamageMultiplier : 1.0f;
        }

    private:
        static bool InRange(double Distance, double Height)
        {
            return Distance > 0.0 && Distance <= ReachCm && std::abs(Height) <= HeightCm;
        }
    };
}
