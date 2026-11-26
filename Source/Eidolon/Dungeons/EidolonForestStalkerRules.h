#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

namespace EidolonForestStalker
{
    enum class Awareness { Returning, Tracking, Searching };
    enum class Move { RootSweep, Pounce };
    enum class Phase { Ready, Windup, Active, Recovery };

    inline constexpr double SearchSeconds = 2.4;
    inline constexpr double SweepRangeCm = 245.0;
    inline constexpr double PounceRangeCm = 530.0;
    inline constexpr double SweepMinCosine = 0.57;
    inline constexpr double MaxHeightDeltaCm = 155.0;
    inline constexpr double SweepWindupSeconds = 0.75;
    inline constexpr double SweepActiveSeconds = 0.22;
    inline constexpr double PounceWindupSeconds = 0.95;
    inline constexpr double PounceActiveSeconds = 0.42;
    inline constexpr double HitRecoverySeconds = 0.82;
    inline constexpr double MissRecoverySeconds = 1.65;
    inline constexpr double PerchCenterHeightCm = 95.0;
    inline constexpr double MinPerchRouteCm = 120.0;
    inline constexpr double MaxPerchRouteCm = 500.0;
    inline constexpr double MinPerchElevationCm = 45.0;
    inline constexpr double MaxPerchElevationCm = 220.0;
    inline constexpr double PerchLandingRadiusCm = 145.0;
    inline constexpr double PerchWindupSeconds = 1.10;
    inline constexpr double PerchTraversalSeconds = 0.85;
    inline constexpr double PerchWaitSeconds = 2.40;
    inline constexpr double CanalMaxGroundStepCm = 30.0;
    inline constexpr double CanalPounceGroundToleranceCm = 10.0;

    inline std::optional<double> CanalRootZ(double CurrentZ, double SupportZ, bool FollowGrade)
    {
        if (!std::isfinite(CurrentZ) || !std::isfinite(SupportZ)) return std::nullopt;
        const double AdjustedZ = SupportZ + PerchCenterHeightCm;
        const double Limit = FollowGrade ? CanalMaxGroundStepCm : CanalPounceGroundToleranceCm;
        if (std::abs(AdjustedZ - CurrentZ) > Limit) return std::nullopt;
        return FollowGrade ? AdjustedZ : CurrentZ;
    }

    inline bool IsPerchRouteValid(double HorizontalCm, double ElevationCm)
    {
        return std::isfinite(HorizontalCm) && std::isfinite(ElevationCm)
            && HorizontalCm >= MinPerchRouteCm && HorizontalCm <= MaxPerchRouteCm
            && ElevationCm >= MinPerchElevationCm && ElevationCm <= MaxPerchElevationCm;
    }

    // Hold ledge height while the body clears the lip, then descend. The
    // climb follows this same authored route in reverse.
    inline double PerchAltitude(double UpperZ, double LowerZ, double Alpha)
    {
        const double Fraction = std::clamp((Alpha - 0.68) / 0.32, 0.0, 1.0);
        const double Smooth = Fraction * Fraction * (3.0 - 2.0 * Fraction);
        return UpperZ + (LowerZ - UpperZ) * Smooth;
    }

    inline bool IsPerchLandingHit(bool ReachedAuthoredLanding, bool Visible,
                                  double DistanceSquaredCm, double HeightFromCenterCm)
    {
        return ReachedAuthoredLanding && Visible && std::isfinite(DistanceSquaredCm)
            && std::isfinite(HeightFromCenterCm) && DistanceSquaredCm >= 0.0
            && DistanceSquaredCm <= PerchLandingRadiusCm * PerchLandingRadiusCm
            && std::abs(HeightFromCenterCm) <= MaxHeightDeltaCm;
    }

    struct PerchTravel
    {
        double Alpha = 0.0;
        bool bTowardUpper = false;
        bool bAborted = false;

        double Next(double DeltaSeconds) const
        {
            if (!std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0) return Alpha;
            const double Step = std::min(0.08, DeltaSeconds / PerchTraversalSeconds);
            return std::clamp(Alpha + (bTowardUpper ? -Step : Step), 0.0, 1.0);
        }

        void Obstructed()
        {
            if (bAborted) return;
            bAborted = true;
            bTowardUpper = !bTowardUpper;
        }

        bool Arrived() const { return bTowardUpper ? Alpha <= 0.0 : Alpha >= 1.0; }
    };

    struct Search
    {
        Awareness Current = Awareness::Returning;
        double Remaining = 0.0;

        void Observe(bool Visible, double DeltaSeconds)
        {
            if (Visible)
            {
                Current = Awareness::Tracking;
                Remaining = SearchSeconds;
                return;
            }

            if (Current == Awareness::Returning) return;
            if (!std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0) return;
            Remaining = std::max(0.0, Remaining - DeltaSeconds);
            Current = Remaining > 0.0 ? Awareness::Searching : Awareness::Returning;
        }
    };

    struct Attack
    {
        Phase Current = Phase::Ready;
        Move Action = Move::RootSweep;
        double Remaining = 0.0;
        double LockedDistanceCm = 0.0;
        bool bHit = false;

        bool Begin(bool Visible, double TargetDistanceCm)
        {
            if (Current != Phase::Ready || !Visible || !std::isfinite(TargetDistanceCm)
                || TargetDistanceCm <= 0.0 || TargetDistanceCm > PounceRangeCm)
            {
                return false;
            }
            Action = TargetDistanceCm <= SweepRangeCm ? Move::RootSweep : Move::Pounce;
            LockedDistanceCm = TargetDistanceCm;
            Current = Phase::Windup;
            Remaining = Action == Move::Pounce ? PounceWindupSeconds : SweepWindupSeconds;
            bHit = false;
            return true;
        }

        double PounceDistance(double DeltaSeconds) const
        {
            if (Current != Phase::Active || Action != Move::Pounce
                || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
            {
                return 0.0;
            }
            return LockedDistanceCm * (std::min(DeltaSeconds, Remaining) / PounceActiveSeconds);
        }

        bool TrySweepHit(bool Visible, double ForwardCm, double SideCm, double HeightCm)
        {
            if (Current != Phase::Active || Action != Move::RootSweep || bHit || !Visible
                || !std::isfinite(ForwardCm) || !std::isfinite(SideCm) || !std::isfinite(HeightCm)
                || std::abs(HeightCm) > MaxHeightDeltaCm)
            {
                return false;
            }
            const double Distance = std::hypot(ForwardCm, SideCm);
            if (Distance <= 0.0 || Distance > SweepRangeCm || ForwardCm < SweepMinCosine * Distance)
                return false;

            bHit = true;
            return true;
        }

        bool StopPounce(bool HitPlayer)
        {
            if (Current != Phase::Active || Action != Move::Pounce) return false;
            bHit = HitPlayer;
            FinishAttack();
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
                    Current = Phase::Active;
                    Remaining = Action == Move::Pounce ? PounceActiveSeconds : SweepActiveSeconds;
                    break;
                case Phase::Active:
                    FinishAttack();
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
            return Current == Phase::Recovery && !bHit ? 1.5 : 1.0;
        }

    private:
        void FinishAttack()
        {
            Current = Phase::Recovery;
            Remaining = bHit ? HitRecoverySeconds : MissRecoverySeconds;
        }
    };
}
