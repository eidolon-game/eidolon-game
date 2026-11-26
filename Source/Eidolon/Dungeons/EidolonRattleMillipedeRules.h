#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace EidolonRattleMillipede
{
    enum class Phase { Ready, Telegraph, Strike, Recovery };
    inline constexpr double TelegraphSeconds = 0.85;
    inline constexpr double StrikeSeconds = 0.25;
    inline constexpr double RecoverySeconds = 1.05;
    inline constexpr double TailCm = -190.0;
    inline constexpr double NoseCm = 55.0;
    inline constexpr double InnerSideCm = 28.0;
    inline constexpr double OuterSideCm = 168.0;
    inline constexpr double HeightCm = 112.0;

    inline std::size_t AdjacentWaypoint(std::size_t Current, std::size_t Count, bool Forward)
    {
        if (Count < 2 || Current >= Count) return Current;
        if (Forward) return Current == Count - 1 ? 0 : Current + 1;
        return Current == 0 ? Count - 1 : Current - 1;
    }

    struct Attack
    {
        Phase Current = Phase::Ready;
        double Remaining = 0.0;
        int LockedSide = 0;
        bool bSpent = false;

        bool Begin(bool Visible, double Along, double Side, double Height)
        {
            if (Current != Phase::Ready || !Visible || !InReach(Along, Side, Height))
                return false;

            Current = Phase::Telegraph;
            Remaining = TelegraphSeconds;
            LockedSide = Side > 0.0 ? 1 : -1;
            bSpent = false;
            return true;
        }

        bool TryHit(bool Visible, double Along, double Side, double Height)
        {
            if (!Visible || Current != Phase::Strike || bSpent
                || !InReach(Along, Side, Height) || Side * LockedSide <= 0.0)
            {
                return false;
            }

            bSpent = true;
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
                    Current = Phase::Strike;
                    Remaining = StrikeSeconds;
                    break;
                case Phase::Strike:
                    Current = Phase::Recovery;
                    Remaining = RecoverySeconds;
                    break;
                case Phase::Recovery:
                    Current = Phase::Ready;
                    LockedSide = 0;
                    break;
                case Phase::Ready:
                    break;
            }
        }

    private:
        static bool InReach(double Along, double Side, double Height)
        {
            return std::isfinite(Along) && std::isfinite(Side) && std::isfinite(Height)
                && Along >= TailCm && Along <= NoseCm
                && std::abs(Side) >= InnerSideCm && std::abs(Side) <= OuterSideCm
                && std::abs(Height) <= HeightCm;
        }
    };
}
