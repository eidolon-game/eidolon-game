#include "../Source/Eidolon/Dungeons/EidolonRiftrootRules.h"

#include <iostream>
#include <limits>

using namespace EidolonRiftroot;

int main()
{
    Attack Wedge;
    const Timing Rock = GetTiming(Move::RockWedge);
    if (!Wedge.Begin(Move::RockWedge) || Wedge.Current != Phase::Aim || Wedge.Remaining != 0.65)
        return 1;
    Wedge.ClipWedgeAt(100.0);
    if (Wedge.LockedWedgeEndCm != WedgeEndCm)
        return 1;
    Wedge.AdvanceTime(Rock.Aim);
    Wedge.ClipWedgeAt(470.0);
    if (Wedge.Current != Phase::Lock || Wedge.Remaining != 0.4 ||
        Wedge.TryHit(true, 220.0, 0.0, 0.0))
    {
        std::cerr << "Rock wedge missed its axis lock or hit during the tell\n";
        return 1;
    }
    Wedge.AdvanceTime(Rock.Telegraph);
    if (Wedge.TryHit(false, 220.0, 0.0, 0.0)
        || Wedge.TryHit(true, WedgeStartCm - 1.0, 0.0, 0.0)
        || Wedge.TryHit(true, 471.0, 0.0, 0.0)
        || Wedge.TryHit(true, 220.0, WedgeHalfWidthCm + 1.0, 0.0)
        || !Wedge.TryHit(true, 470.0, WedgeHalfWidthCm, HitHeightCm)
        || Wedge.TryHit(true, 220.0, 0.0, 0.0))
    {
        std::cerr << "Rock wedge clipping, lateral escape or single impact failed\n";
        return 1;
    }
    Wedge.AdvanceTime(Rock.Active);
    if (Wedge.Current != Phase::Recovery || Wedge.Remaining != 1.2
        || Wedge.DamageMultiplier(0.0, 180.0) <= 1.0 || Wedge.DamageMultiplier(200.0, 10.0) != 1.0)
        return 1;

    Attack Obscured;
    if (!Obscured.Begin(Move::RockWedge)) return 1;
    Obscured.AdvanceTime(Rock.Aim);
    Obscured.ClipWedgeAt(49.0);
    Obscured.AdvanceTime(Rock.Telegraph);
    if (Obscured.TryHit(true, WedgeStartCm, 0.0, 0.0)) return 1;

    for (const int Side : {-1, 1})
    {
        Attack Roots;
        const Timing Draw = GetTiming(Move::RootDraw);
        if (!Roots.Begin(Move::RootDraw, Side) || Roots.Current != Phase::Lock || Roots.Remaining < 1.0
            || Roots.TryHit(true, 110.0, Side * 220.0, 0.0))
            return 1;
        Roots.AdvanceTime(Draw.Telegraph);
        if (Roots.TryHit(true, 100.0, -Side * 220.0, 0.0)
            || Roots.TryHit(true, 100.0, Side * (RootInnerCm - 1.0), 0.0)
            || Roots.TryHit(true, RootBackCm - 1.0, Side * 220.0, 0.0)
            || Roots.TryHit(true, 140.0, Side * 220.0, HitHeightCm + 1.0)
            || Roots.TryHit(true, 100.0, std::numeric_limits<double>::quiet_NaN(), 0.0)
            || !Roots.TryHit(true, RootFrontCm, Side * RootOuterCm, 0.0)
            || Roots.TryHit(true, 150.0, Side * 300.0, 0.0))
        {
            std::cerr << "Root draw crossed its indicated flank, reached the center, or repeated damage\n";
            return 1;
        }
        Roots.AdvanceTime(Draw.Active);
        if (Roots.DamageMultiplier(-85.0, Side * 180.0) <= 1.0
            || Roots.DamageMultiplier(-85.0, -Side * 180.0) != 1.0)
            return 1;
    }

    for (const Phase Crossing : {Phase::Aim, Phase::Lock, Phase::Active, Phase::Recovery})
    {
        Attack Boss;
        Boss.ObserveHealth(127.0, 280.0);
        if (Boss.bAdvancePending || Boss.Begin(Move::FaultAdvance) || !Boss.Begin(Move::RockWedge))
            return 1;
        if (Crossing != Phase::Aim) Boss.AdvanceTime(Rock.Aim);
        if (Crossing == Phase::Active || Crossing == Phase::Recovery) Boss.AdvanceTime(Rock.Telegraph);
        if (Crossing == Phase::Recovery) Boss.AdvanceTime(Rock.Active);
        Boss.ObserveHealth(126.0, 280.0);
        if (!Boss.bAdvancePending || Boss.Current != Crossing || Boss.Begin(Move::FaultAdvance))
            return 1;
        while (Boss.Current != Phase::Ready) Boss.AdvanceTime(20.0);
        if (Boss.Begin(Move::RootDraw) || !Boss.Begin(Move::FaultAdvance) || Boss.Remaining < 1.4)
            return 1;
        Boss.AdvanceTime(GetTiming(Move::FaultAdvance).Telegraph);
        if (Boss.TryHit(true, 220.0, 250.0, 0.0) || !Boss.StopAdvance() || Boss.StopAdvance()
            || Boss.Current != Phase::Recovery)
            return 1;
        Boss.AdvanceTime(GetTiming(Move::FaultAdvance).Recovery);
        Boss.ObserveHealth(5.0, 280.0);
        if (!Boss.bAdvanceConsumed || Boss.bAdvancePending || Boss.Begin(Move::FaultAdvance)
            || !Boss.Begin(Move::RootDraw))
        {
            std::cerr << "Blocked fault advance requeued, skipped safety pause, or prevented normal attacks\n";
            return 1;
        }
    }

    Attack MissingAnchor;
    MissingAnchor.ObserveHealth(120.0, 280.0);
    if (!MissingAnchor.Begin(Move::FaultAdvance)) return 1;
    while (MissingAnchor.Current != Phase::Ready) MissingAnchor.AdvanceTime(20.0);
    if (!MissingAnchor.bAdvanceConsumed || MissingAnchor.bAdvancePending) return 1;

    Attack Lethal;
    Lethal.ObserveHealth(126.0, 280.0);
    Lethal.ObserveHealth(0.0, 280.0);
    if (Lethal.bAdvancePending || Lethal.Begin(Move::FaultAdvance)) return 1;

    std::cout << "Riftroot: clipped wedge, one-sided roots, both escapes, 45% single shift, blocked and lethal cases passed\n";
}
