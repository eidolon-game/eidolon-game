#pragma once

#include <algorithm>
#include <cmath>

namespace EidolonMortarLeech
{
    enum class Phase { Ready, Telegraph, Strike, Recovery };
    inline constexpr double MaxReachCm = 480.0;
    inline constexpr double HalfWidthCm = 30.0;
    inline constexpr double HeightCm = 80.0;
    inline constexpr double TelegraphSeconds = 0.85;
    inline constexpr double StrikeSeconds = 0.18;
    inline constexpr double RecoverySeconds = 1.20;

    struct Attack
    {
        Phase Current = Phase::Ready;
        double Remaining = 0.0;
        double LengthCm = 0.0;

        bool Begin(double DistanceCm, double HeightOffsetCm, double ClearCm)
        {
            if (Current != Phase::Ready || !std::isfinite(DistanceCm)
                || !std::isfinite(HeightOffsetCm) || !std::isfinite(ClearCm)
                || DistanceCm <= 0.0 || DistanceCm > MaxReachCm
                || DistanceCm > ClearCm || std::abs(HeightOffsetCm) > HeightCm)
            {
                return false;
            }

            LengthCm = std::min(ClearCm, MaxReachCm);
            Current = Phase::Telegraph;
            Remaining = TelegraphSeconds;
            return true;
        }

        // Resolve damage once, on the transition from Telegraph to Strike.
        bool Advance(double DeltaSeconds)
        {
            if (Current == Phase::Ready || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
                return false;

            Remaining = std::max(0.0, Remaining - DeltaSeconds);
            if (Remaining > 0.0) return false;

            if (Current == Phase::Telegraph)
            {
                Current = Phase::Strike;
                Remaining = StrikeSeconds;
                return true;
            }
            if (Current == Phase::Strike)
            {
                Current = Phase::Recovery;
                Remaining = RecoverySeconds;
            }
            else
            {
                Current = Phase::Ready;
                LengthCm = 0.0;
            }
            return false;
        }

        bool Contains(double AlongCm, double SideCm, double HeightOffsetCm) const
        {
            return Current == Phase::Strike && AlongCm >= 0.0 && AlongCm <= LengthCm
                && std::abs(SideCm) <= HalfWidthCm && std::abs(HeightOffsetCm) <= HeightCm;
        }
    };
}
