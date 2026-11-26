#pragma once

#include <algorithm>
#include <cmath>

namespace EidolonRiftroot
{
    enum class Move { RockWedge, RootDraw, FaultAdvance };
    enum class Phase { Ready, Aim, Lock, Active, Recovery };

    inline constexpr double ShiftHealthFraction = 0.45;
    inline constexpr double WedgeStartCm = 150.0;
    inline constexpr double WedgeEndCm = 860.0;
    inline constexpr double WedgeHalfWidthCm = 125.0;
    inline constexpr double RootBackCm = -245.0;
    inline constexpr double RootFrontCm = 340.0;
    inline constexpr double RootInnerCm = 185.0;
    inline constexpr double RootOuterCm = 505.0;
    inline constexpr double HitHeightCm = 170.0;

    struct Timing { double Aim; double Telegraph; double Active; double Recovery; };

    inline constexpr Timing GetTiming(Move Kind)
    {
        switch (Kind)
        {
            case Move::RockWedge: return {0.65, 0.40, 0.22, 1.20};
            case Move::RootDraw: return {0.0, 1.05, 0.32, 1.00};
            case Move::FaultAdvance: return {0.0, 1.45, 0.90, 0.95};
        }
        return {};
    }

    struct Attack
    {
        Move Kind = Move::RockWedge;
        Phase Current = Phase::Ready;
        double Remaining = 0.0;
        double LockedWedgeEndCm = WedgeEndCm;
        int RootSide = 1;
        bool bAdvancePending = false;
        bool bAdvanceConsumed = false;
        bool bHitSpent = false;

        void ObserveHealth(double Health, double Maximum)
        {
            if (!std::isfinite(Health) || !std::isfinite(Maximum) || Maximum <= 0.0) return;
            if (Health <= 0.0)
            {
                bAdvancePending = false;
                return;
            }
            if (!bAdvanceConsumed && Health <= Maximum * ShiftHealthFraction)
                bAdvancePending = true;
        }

        bool Begin(Move Requested, int Side = 1)
        {
            if (Current != Phase::Ready || (Requested == Move::FaultAdvance) != bAdvancePending)
                return false;

            const Timing Profile = GetTiming(Requested);
            if (Profile.Active <= 0.0) return false;

            Kind = Requested;
            RootSide = Side < 0 ? -1 : 1;
            LockedWedgeEndCm = WedgeEndCm;
            bHitSpent = false;
            Current = Profile.Aim > 0.0 ? Phase::Aim : Phase::Lock;
            Remaining = Profile.Aim > 0.0 ? Profile.Aim : Profile.Telegraph;
            return true;
        }

        void ClipWedgeAt(double ClearDistanceCm)
        {
            if (Kind == Move::RockWedge && Current == Phase::Lock && std::isfinite(ClearDistanceCm))
                LockedWedgeEndCm = std::clamp(ClearDistanceCm, WedgeStartCm, WedgeEndCm);
        }

        bool TryHit(bool Visible, double Forward, double Side, double Height)
        {
            if (!Visible || Current != Phase::Active || bHitSpent || Kind == Move::FaultAdvance
                || !std::isfinite(Forward) || !std::isfinite(Side) || !std::isfinite(Height)
                || std::abs(Height) > HitHeightCm)
            {
                return false;
            }

            bool InVolume;
            if (Kind == Move::RockWedge)
            {
                InVolume = LockedWedgeEndCm > WedgeStartCm && Forward >= WedgeStartCm
                    && Forward <= LockedWedgeEndCm && std::abs(Side) <= WedgeHalfWidthCm;
            }
            else
            {
                const double SideOfDraw = Side * RootSide;
                InVolume = Forward >= RootBackCm && Forward <= RootFrontCm
                    && SideOfDraw >= RootInnerCm && SideOfDraw <= RootOuterCm;
            }

            if (InVolume) bHitSpent = true;
            return InVolume;
        }

        bool StopAdvance()
        {
            if (Current != Phase::Active || Kind != Move::FaultAdvance) return false;
            Current = Phase::Recovery;
            Remaining = GetTiming(Kind).Recovery;
            return true;
        }

        void AdvanceTime(double DeltaSeconds)
        {
            if (Current == Phase::Ready || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0) return;
            Remaining = std::max(0.0, Remaining - DeltaSeconds);
            if (Remaining > 0.0) return;

            const Timing Profile = GetTiming(Kind);
            switch (Current)
            {
                case Phase::Aim: Current = Phase::Lock; Remaining = Profile.Telegraph; break;
                case Phase::Lock: Current = Phase::Active; Remaining = Profile.Active; break;
                case Phase::Active: Current = Phase::Recovery; Remaining = Profile.Recovery; break;
                case Phase::Recovery:
                    if (Kind == Move::FaultAdvance)
                    {
                        bAdvanceConsumed = true;
                        bAdvancePending = false;
                    }
                    Current = Phase::Ready;
                    break;
                case Phase::Ready:
                    break;
            }
        }

        double DamageMultiplier(double Forward, double Side) const
        {
            if (Current != Phase::Recovery || !std::isfinite(Forward) || !std::isfinite(Side))
                return 1.0;
            if (Kind == Move::RockWedge && Forward < 170.0 && std::abs(Side) >= 110.0)
                return 1.4;
            if (Kind == Move::RootDraw && Forward <= 0.0 && Side * RootSide >= 105.0)
                return 1.35;
            return 1.0;
        }
    };
}
