#pragma once

#include <algorithm>
#include <cmath>

namespace EidolonCenserHagfish
{
    enum class Phase { Ready, Telegraph, Active, Recovery };
    inline constexpr double TelegraphSeconds = 1.10;
    inline constexpr double ActiveSeconds = 0.45;
    inline constexpr double RecoverySeconds = 1.40;
    inline constexpr double MaxLockRangeCm = 540.0;
    inline constexpr double MaxStripLengthCm = 620.0;
    inline constexpr double HalfWidthCm = 46.0;
    inline constexpr double HeightToleranceCm = 112.0;

    struct Cast
    {
        Phase Current = Phase::Ready;
        double Remaining = 0.0;
        double LengthCm = 0.0;
        bool bDamageSpent = false;

        bool HoldsWorldClaim() const
        {
            return Current == Phase::Telegraph || Current == Phase::Active;
        }

        bool Begin(bool WorldClaimAvailable, double TargetDistanceCm, double UnobstructedDistanceCm)
        {
            if (Current != Phase::Ready || !WorldClaimAvailable
                || !std::isfinite(TargetDistanceCm) || TargetDistanceCm <= 0.0 || TargetDistanceCm > MaxLockRangeCm
                || !std::isfinite(UnobstructedDistanceCm) || UnobstructedDistanceCm < TargetDistanceCm)
            {
                return false;
            }
            Current = Phase::Telegraph;
            Remaining = TelegraphSeconds;
            LengthCm = std::min(UnobstructedDistanceCm, MaxStripLengthCm);
            bDamageSpent = false;
            return true;
        }

        bool TryHit(double ForwardCm, double SideCm, double HeightCm)
        {
            if (Current != Phase::Active || bDamageSpent ||
                !std::isfinite(ForwardCm) || !std::isfinite(SideCm) || !std::isfinite(HeightCm)
                || ForwardCm < 0.0 || ForwardCm > LengthCm || std::abs(SideCm) > HalfWidthCm
                || std::abs(HeightCm) > HeightToleranceCm)
            {
                return false;
            }
            bDamageSpent = true;
            return true;
        }

        void AdvanceTime(double DeltaSeconds)
        {
            if (Current == Phase::Ready || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0) return;
            Remaining = std::max(0.0, Remaining - DeltaSeconds);
            if (Remaining > 0.0) return;

            switch (Current)
            {
                case Phase::Telegraph:
                    Current = Phase::Active;
                    Remaining = ActiveSeconds;
                    break;
                case Phase::Active:
                    Current = Phase::Recovery;
                    Remaining = RecoverySeconds;
                    break;
                case Phase::Recovery:
                    Current = Phase::Ready;
                    LengthCm = 0.0;
                    break;
                case Phase::Ready:
                    break;
            }
        }
    };
}
