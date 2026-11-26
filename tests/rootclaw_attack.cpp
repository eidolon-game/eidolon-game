#include "../Source/Eidolon/Dungeons/EidolonRootclawRules.h"

#include <iostream>

using namespace EidolonRootclaw;

int main()
{
    for (const int Side : {-1, 1})
    {
        Attack Boss;
        Boss.ObserveHealth(141.0, 280.0);
        if (Boss.bTransitionPending || Boss.Begin(Move::LaneStep, true) || !Boss.Begin(Move::Sweep, false, Side))
            return 1;

        const Timing Sweep = GetTiming(Move::Sweep, false);
        Boss.AdvanceTime(Sweep.Aim);
        Boss.ObserveHealth(140.0, 280.0);
        if (Boss.Current != Phase::Lock || !Boss.bTransitionPending || Boss.bSecondPhase
            || Boss.TryHit(true, 300.0, Side * 240.0, 0.0, Sweep.Active)
            || Boss.Begin(Move::LaneStep, true))
        {
            std::cerr << "Threshold interrupted timing or the locked telegraph damaged early\n";
            return 1;
        }
        Boss.AdvanceTime(Sweep.Lock);
        if (Boss.TryHit(false, 240.0, Side * 240.0, 0.0, Sweep.Active)
            || Boss.TryHit(true, 240.0, -Side * 240.0, 0.0, Sweep.Active / 8.0)
            || Boss.TryHit(true, SweepRangeCm + 1.0, 0.0, 0.0, Sweep.Active)
            || !Boss.TryHit(true, 240.0, Side * 240.0, 0.0, Sweep.Active / 3.0)
            || Boss.TryHit(true, 240.0, Side * 240.0, 0.0, Sweep.Active))
        {
            std::cerr << "Sweep interval, dodge, wall or single impact handling failed\n";
            return 1;
        }

        Boss.AdvanceTime(Sweep.Active);
        if (Boss.Current != Phase::Recovery || Boss.DamageMultiplier() <= 1.0 ||
            Boss.Begin(Move::LaneStep, true))
            return 1;
        Boss.AdvanceTime(Sweep.Recovery);
        if (Boss.Begin(Move::Thrust, false) || Boss.Begin(Move::LaneStep, false) ||
            !Boss.Begin(Move::LaneStep, true) || Boss.Remaining < 1.2)
        {
            std::cerr << "Mandatory transition did not wait for recovery and a valid lane\n";
            return 1;
        }

        Boss.AdvanceTime(GetTiming(Move::LaneStep, true).Lock);
        if (Boss.TryHit(true, 1.0, 1.0, 0.0, 0.1) || !Boss.StopLaneStep() || Boss.StopLaneStep()
            || Boss.Begin(Move::Sweep, false) || Boss.bSecondPhase)
        {
            std::cerr << "Blocked lane step attacked or skipped recovery\n";
            return 1;
        }
        Boss.AdvanceTime(GetTiming(Move::LaneStep, true).Recovery);
        Boss.ObserveHealth(60.0, 280.0);
        if (Boss.Current != Phase::Ready || !Boss.bSecondPhase || Boss.bTransitionPending
            || !Boss.Begin(Move::Thrust, false))
        {
            std::cerr << "Second phase was absent, duplicated or blocked normal attacks\n";
            return 1;
        }

        const Timing Thrust = GetTiming(Move::Thrust, false);
        Boss.AdvanceTime(Thrust.Aim);
        Boss.AdvanceTime(Thrust.Lock);
        if (Boss.TryHit(true, 400.0, ThrustHalfWidthCm + 1.0, 0.0, Thrust.Active)
            || !Boss.TryHit(true, ThrustRangeCm, -ThrustHalfWidthCm, MaxHeightCm, Thrust.Active)
            || Boss.TryHit(true, 1.0, 1.0, 0.0, Thrust.Active))
        {
            std::cerr << "Thrust width, range, height or single impact handling failed\n";
            return 1;
        }
    }

    Attack Lethal;
    Lethal.ObserveHealth(139.0, 280.0);
    Lethal.ObserveHealth(0.0, 280.0);
    if (Lethal.bTransitionPending || Lethal.bSecondPhase || Lethal.Begin(Move::LaneStep, true))
    {
        std::cerr << "Lethal crossing queued a phase change\n";
        return 1;
    }

    std::cout << "Rootclaw: both sweeps, narrow thrust, half-health transition, blocked lane and lethal crossing passed\n";
}
