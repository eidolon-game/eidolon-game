#pragma once

#include <algorithm>
#include <cmath>

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
