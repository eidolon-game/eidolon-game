#pragma once

#include <algorithm>
#include <cmath>

namespace EidolonRootclaw
{
    enum class Move { Sweep, Thrust, LaneStep };
    enum class Phase { Ready, Aim, Lock, Active, Recovery };

    inline constexpr double SweepRangeCm = 420.0;
    inline constexpr double ThrustRangeCm = 520.0;
    inline constexpr double ThrustHalfWidthCm = 85.0;
    inline constexpr double MaxHeightCm = 175.0;
    inline constexpr double SweepArcHalfDegrees = 70.0;
    inline constexpr double SweepContactHalfDegrees = 17.0;

    struct Timing
    {
        double Aim;
        double Lock;
        double Active;
        double Recovery;
    };

    inline constexpr Timing GetTiming(Move Action, bool Transition)
    {
        switch (Action)
        {
            case Move::Sweep: return {0.55, 0.35, 0.25, 1.10};
            case Move::Thrust: return {0.50, 0.25, 0.18, 0.75};
            case Move::LaneStep: return {0.0, Transition ? 1.45 : 1.20, 0.70, 0.80};
        }
        return {};
    }

    struct Attack
    {
        Phase Current = Phase::Ready;
        Move Action = Move::Sweep;
        double Remaining = 0.0;
        int SweepSign = 1;
        bool bTransitionPending = false;
        bool bSecondPhase = false;
        bool bSpent = false;

        void ObserveHealth(double Health, double MaxHealth)
        {
            if (!std::isfinite(Health) || !std::isfinite(MaxHealth) || MaxHealth <= 0.0) return;
            if (Health <= 0.0)
            {
                bTransitionPending = false;
                return;
            }
            if (!bSecondPhase && Health <= MaxHealth / 2.0)
                bTransitionPending = true;
        }

        bool Begin(Move Requested, bool HasLane, int DirectionSign = 1)
        {
            if (Current != Phase::Ready || (bTransitionPending && Requested != Move::LaneStep))
                return false;
            if (Requested == Move::LaneStep && (!HasLane || (!bSecondPhase && !bTransitionPending)))
                return false;

            const Timing Profile = GetTiming(Requested, bTransitionPending);
            if (Profile.Active <= 0.0) return false;
            Action = Requested;
            SweepSign = DirectionSign < 0 ? -1 : 1;
            bSpent = false;
            Current = Profile.Aim > 0.0 ? Phase::Aim : Phase::Lock;
            Remaining = Profile.Aim > 0.0 ? Profile.Aim : Profile.Lock;
            return true;
        }

        double SweepAngle(double DeltaSeconds = 0.0) const
        {
            const double Duration = GetTiming(Move::Sweep, false).Active;
            const double Fraction = Current == Phase::Active
                ? std::clamp((Duration - Remaining + DeltaSeconds) / Duration, 0.0, 1.0) : 0.0;
            return SweepSign * SweepArcHalfDegrees * (1.0 - 2.0 * Fraction);
        }

        bool TryHit(bool Visible, double Forward, double Side, double Height, double DeltaSeconds)
        {
            if (!Visible || Current != Phase::Active || bSpent || Action == Move::LaneStep
                || !std::isfinite(Forward) || !std::isfinite(Side) || !std::isfinite(Height)
                || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0 || std::abs(Height) > MaxHeightCm)
            {
                return false;
            }

            if (Action == Move::Thrust)
            {
                if (Forward < 0.0 || Forward > ThrustRangeCm || std::abs(Side) > ThrustHalfWidthCm)
                    return false;
            }
            else
            {
                if (Forward <= 0.0 || std::hypot(Forward, Side) > SweepRangeCm)
                    return false;
                constexpr double RadiansToDegrees = 57.29577951308232;
                const double Angle = std::atan2(Side, Forward) * RadiansToDegrees;
                const double Start = SweepAngle();
                const double End = SweepAngle(DeltaSeconds);
                if (Angle < std::min(Start, End) - SweepContactHalfDegrees
                    || Angle > std::max(Start, End) + SweepContactHalfDegrees)
                {
                    return false;
                }
            }

            bSpent = true;
            return true;
        }

        bool StopLaneStep()
        {
            if (Current != Phase::Active || Action != Move::LaneStep) return false;
            Current = Phase::Recovery;
            Remaining = GetTiming(Action, bTransitionPending).Recovery;
            return true;
        }

        void AdvanceTime(double DeltaSeconds)
        {
            if (Current == Phase::Ready || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0)
                return;

            Remaining = std::max(0.0, Remaining - DeltaSeconds);
            if (Remaining > 0.0) return;
            const Timing Profile = GetTiming(Action, bTransitionPending);
            switch (Current)
            {
                case Phase::Aim: Current = Phase::Lock; Remaining = Profile.Lock; break;
                case Phase::Lock: Current = Phase::Active; Remaining = Profile.Active; break;
                case Phase::Active: Current = Phase::Recovery; Remaining = Profile.Recovery; break;
                case Phase::Recovery:
                    if (Action == Move::LaneStep && bTransitionPending)
                    {
                        bSecondPhase = true;
                        bTransitionPending = false;
                    }
                    Current = Phase::Ready;
                    break;
                case Phase::Ready: break;
            }
        }

        double DamageMultiplier() const
        {
            return Action == Move::Sweep && Current == Phase::Recovery ? 1.35 : 1.0;
        }
    };
}
