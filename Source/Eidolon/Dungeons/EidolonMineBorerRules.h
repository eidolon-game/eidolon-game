#pragma once

#include <algorithm>
#include <cmath>

namespace EidolonMineBorer
{
    inline constexpr double ChargeLimitCm = 770.0;
    inline constexpr double HazardHalfWidthCm = 225.0;
    inline constexpr double BodyHalfLengthCm = 238.0;
    inline constexpr double BodyHalfWidthCm = 205.0;
    inline constexpr double BodyHalfHeightCm = 146.0;
    inline constexpr double ApproachSeconds = 0.50;
    inline constexpr double ApproachSpeedCm = 155.0;
    inline constexpr double WindupSeconds = 0.90;
    inline constexpr double ChargeSeconds = 1.05;
    inline constexpr double ImpactSeconds = 0.25;
    inline constexpr double RecoverySeconds = 1.90;
    inline constexpr double DebrisReachCm = 335.0;

    enum class Phase { Ready, Approach, Windup, Charge, Impact, Recovery };

    struct Attack
    {
        Phase Current = Phase::Ready;
        double Remaining = 0.0;

        bool Begin(bool Visible, double Ahead, double Across, double Height)
        {
            if (Current != Phase::Ready || !Visible
                || !std::isfinite(Ahead) || !std::isfinite(Across) || !std::isfinite(Height)
                || Ahead < 260.0 || Ahead > 1010.0
                || std::abs(Across) > HazardHalfWidthCm || std::abs(Height) > 150.0)
                return false;

            Current = Phase::Approach;
            Remaining = ApproachSeconds;
            return true;
        }

        double ChargeDistance(double DeltaSeconds) const
        {
            if (Current != Phase::Charge || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
                return 0.0;
            return ChargeLimitCm * (std::min(DeltaSeconds, Remaining) / ChargeSeconds);
        }

        void Stop()
        {
            if (Current != Phase::Charge) return;
            Current = Phase::Impact;
            Remaining = ImpactSeconds;
        }

        void Advance(double DeltaSeconds)
        {
            if (Current == Phase::Ready || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
                return;
            Remaining = std::max(0.0, Remaining - DeltaSeconds);
            if (Remaining > 0.0) return;

            switch (Current)
            {
                case Phase::Approach: Current = Phase::Windup; Remaining = WindupSeconds; break;
                case Phase::Windup: Current = Phase::Charge; Remaining = ChargeSeconds; break;
                case Phase::Charge: Stop(); break;
                case Phase::Impact: Current = Phase::Recovery; Remaining = RecoverySeconds; break;
                case Phase::Recovery: Current = Phase::Ready; break;
                case Phase::Ready: break;
            }
        }

        bool DebrisCanHit(double Ahead, double Across, double Height) const
        {
            return Current == Phase::Impact && Ahead >= BodyHalfLengthCm
                && Ahead <= BodyHalfLengthCm + DebrisReachCm
                && std::abs(Across) <= HazardHalfWidthCm && std::abs(Height) <= 150.0;
        }

        double DamageMultiplier(double ForwardDot) const
        {
            return Current == Phase::Recovery && ForwardDot < -0.40 ? 1.65 : 0.80;
        }
    };
}
