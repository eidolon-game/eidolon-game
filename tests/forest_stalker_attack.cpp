#include "../Source/Eidolon/Dungeons/EidolonForestStalkerRules.h"

#include <iostream>
#include <limits>

using namespace EidolonForestStalker;

int main()
{
    Search AwarenessState;
    AwarenessState.Observe(false, 1.0);
    if (AwarenessState.Current != Awareness::Returning) return 1;
    AwarenessState.Observe(true, 1.0);
    if (AwarenessState.Current != Awareness::Tracking || AwarenessState.Remaining != SearchSeconds) return 1;
    AwarenessState.Observe(false, 0.7);
    if (AwarenessState.Current != Awareness::Searching) return 1;
    const double BeforeInvalidTime = AwarenessState.Remaining;
    AwarenessState.Observe(false, -1.0);
    if (AwarenessState.Remaining != BeforeInvalidTime) return 1;
    AwarenessState.Observe(true, 0.1);
    AwarenessState.Observe(false, SearchSeconds);
    if (AwarenessState.Current != Awareness::Returning || AwarenessState.Remaining != 0.0)
    {
        std::cerr << "Search did not return after its last sighting expired\n";
        return 1;
    }

    Attack Sweep;
    if (Sweep.Begin(false, 200.0) || Sweep.Begin(true, PounceRangeCm + 1.0)
        || !Sweep.Begin(true, SweepRangeCm) || Sweep.Action != Move::RootSweep
        || Sweep.TrySweepHit(true, 150.0, 30.0, 10.0))
    {
        std::cerr << "Unseen target, range or harmless sweep windup failed\n";
        return 1;
    }
    Sweep.AdvanceTime(SweepWindupSeconds);
    if (Sweep.TrySweepHit(false, 150.0, 30.0, 10.0) || Sweep.TrySweepHit(true, -20.0, 0.0, 0.0)
        || Sweep.TrySweepHit(true, 10.0, 150.0, 0.0) || Sweep.TrySweepHit(true, 160.0, 0.0, MaxHeightDeltaCm + 1.0)
        || !Sweep.TrySweepHit(true, 180.0, 30.0, 20.0) || Sweep.TrySweepHit(true, 180.0, 30.0, 20.0))
    {
        std::cerr << "Flanking, occlusion, height or single-hit sweep failed\n";
        return 1;
    }
    Sweep.AdvanceTime(SweepActiveSeconds);
    if (Sweep.Current != Phase::Recovery || Sweep.Remaining != HitRecoverySeconds || Sweep.DamageMultiplier() != 1.0)
        return 1;

    Attack Pounce;
    if (!Pounce.Begin(true, PounceRangeCm) || Pounce.Action != Move::Pounce
        || Pounce.Begin(true, 300.0) || Pounce.PounceDistance(1.0) != 0.0 || Pounce.StopPounce(true))
        return 1;
    Pounce.AdvanceTime(PounceWindupSeconds);
    const double FirstPart = Pounce.PounceDistance(PounceActiveSeconds / 2.0);
    Pounce.AdvanceTime(PounceActiveSeconds / 2.0);
    if (std::abs(FirstPart + Pounce.PounceDistance(1.0) - PounceRangeCm) > 1e-9
        || Pounce.PounceDistance(std::numeric_limits<double>::quiet_NaN()) != 0.0
        || !Pounce.StopPounce(false) || Pounce.StopPounce(true)
        || Pounce.Remaining != MissRecoverySeconds || Pounce.DamageMultiplier() <= 1.0)
    {
        std::cerr << "Pounce did not keep locked travel or expose joints after missing\n";
        return 1;
    }
    Pounce.AdvanceTime(MissRecoverySeconds);
    if (!Pounce.Begin(true, 300.0)) return 1;
    Pounce.AdvanceTime(PounceWindupSeconds);
    if (!Pounce.StopPounce(true) || Pounce.Remaining != HitRecoverySeconds)
        return 1;

    const double PerchLower = PerchCenterHeightCm;
    const double PerchUpper = PerchLower + 180.0;
    // Authored floor 13 has a 100 cm ledge lip and up to 124 cm swept
    // support envelope in its diagonal return across a 350 cm horizontal run.
    const double ClearLipAlpha = (100.0 + 124.0) / 350.0;
    if (!IsPerchRouteValid(350.0, 180.0) || !IsPerchRouteValid(std::hypot(200.0, 350.0), 180.0)
        || IsPerchRouteValid(350.0, -180.0) || IsPerchRouteValid(MaxPerchRouteCm + 1.0, 150.0)
        || PerchAltitude(PerchUpper, PerchLower, ClearLipAlpha) != PerchUpper
        || PerchAltitude(PerchUpper, PerchLower, 1.0) != PerchLower)
    {
        std::cerr << "Authored perch route bounds, endpoints or ledge clearance failed\n";
        return 1;
    }
    PerchTravel Climb{1.0, true, false};
    if (Climb.Arrived() || Climb.Next(-1.0) != 1.0 || Climb.Next(100.0) < 0.91)
        return 1;
    Climb.Alpha = 0.45;
    Climb.Obstructed();
    if (!Climb.bAborted || Climb.bTowardUpper || Climb.Next(0.1) <= Climb.Alpha)
        return 1;
    Climb.Obstructed();
    if (Climb.bTowardUpper) return 1;
    PerchTravel Drop{0.55, false, false};
    Drop.Obstructed();
    if (!Drop.bAborted || !Drop.bTowardUpper || Drop.Next(0.1) >= Drop.Alpha
        || IsPerchLandingHit(false, true, 0.0, 0.0) || IsPerchLandingHit(true, false, 0.0, 0.0)
        || IsPerchLandingHit(true, true, PerchLandingRadiusCm * PerchLandingRadiusCm + 1.0, 0.0)
        || IsPerchLandingHit(true, true, 0.0, MaxHeightDeltaCm + 1.0)
        || !IsPerchLandingHit(true, true, PerchLandingRadiusCm * PerchLandingRadiusCm, 0.0))
    {
        std::cerr << "Blocked drop did not retreat or could damage outside its landed circle\n";
        return 1;
    }

    const auto TenCm = CanalRootZ(95.0, 10.0, true);
    const auto UpRamp = TenCm ? CanalRootZ(*TenCm, 30.0, true) : std::nullopt;
    const auto DownRamp = UpRamp ? CanalRootZ(*UpRamp, 0.0, true) : std::nullopt;
    const auto FlatPounce = CanalRootZ(95.0, 10.0, false);
    if (!TenCm || *TenCm != 105.0 || !UpRamp || *UpRamp != 125.0
        || !DownRamp || *DownRamp != 95.0 || !FlatPounce || *FlatPounce != 95.0
        || CanalRootZ(95.0, 31.0, true) || CanalRootZ(95.0, -31.0, true)
        || CanalRootZ(95.0, 11.0, false) || CanalRootZ(95.0, -11.0, false)
        || CanalRootZ(95.0, std::numeric_limits<double>::quiet_NaN(), true)
        || CanalRootZ(std::numeric_limits<double>::infinity(), 10.0, true))
    {
        std::cerr << "Canal support lost height, followed a cliff, or bent a locked pounce\n";
        return 1;
    }

    std::cout << "Forest Stalker: search, sweep, locked pounce, perch, landing and canal support passed\n";
}
