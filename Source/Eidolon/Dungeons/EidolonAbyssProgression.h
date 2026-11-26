#pragma once

#include <cstdint>
#include <optional>

namespace EidolonAbyss
{
    inline constexpr int Chapter = 4;
    inline constexpr int SluiceFloor = 4;
    inline constexpr int MarrowFloor = 5;
    inline constexpr int CounterweightFloor = 6;
    inline constexpr int MirrorFloor = 7;
    inline constexpr int PincerWinchFloor = 8;
    inline constexpr int BranchLatchFloor = 9;
    inline constexpr int RootclawFloor = 10;
    inline constexpr int ObliqueClearingFloor = 12;
    inline constexpr int BranchTerraceFloor = 13;
    inline constexpr int FernBasinFloor = 14;
    inline constexpr int HollowTrunkFloor = 15;
    inline constexpr int RootletCanalFloor = 16;
    inline constexpr int SuspendedTrunksFloor = 17;
    inline constexpr int DoubleClearingFloor = 18;
    inline constexpr std::uint8_t DoubleClearingLatchMaxStep = 2;
    inline constexpr int VeinOverlookFloor = 19;
    inline constexpr int RiftrootFloor = 20;
    inline constexpr int SpeakerFloor = 90;
    inline constexpr int GuardianFloor = 100;

    enum class EventKind
    {
        FloorCleared,
        SluicesOpened,
        MarrowLatchOpened,
        CounterweightInstalled,
        MirrorsAligned,
        PincerWinchOpened,
        BranchLatchSelected,
        RootclawDefeated,
        RicordaHeard,
        GuardianDefeated,
        FloorTraversed,
        BranchStairReleased,
        FernBasinDrained,
        HollowTrunkPinSeated,
        RootletCounterweightReleased,
        SuspendedBridgeAligned,
        DoubleClearingLatchOpened,
        RiftrootDefeated
    };

    enum class Producer
    {
        Encounter,
        Puzzle,
        StorySequence,
        Traversal
    };

    enum class Target
    {
        FloorEncounter,
        SluiceControls,
        MarrowLatch,
        CounterweightWinch,
        MirrorControls,
        PincerWinch,
        BranchLatches,
        Rootclaw,
        NamelessSpeaker,
        AbyssGuardian,
        TraversalExit,
        BranchStairRelease,
        FernBasinDrains,
        HollowTrunkSocket,
        RootletCounterweight,
        SuspendedBridgeControl,
        DoubleClearingLatch,
        Riftroot
    };

    struct Progress
    {
        int CompletedFloor = 0;
        bool RicordaHeard = false;
    };

    struct Event
    {
        int ActiveChapter;
        int AuthenticatedFloor;
        int TargetFloor;
        int ExpectedRevision;
        EventKind Kind;
        Producer Source;
        Target Subject;
    };

    inline bool IsValid(const Progress& State)
    {
        return State.CompletedFloor >= 0 && State.CompletedFloor <= GuardianFloor
            && (State.CompletedFloor < SpeakerFloor || State.RicordaHeard)
            && (!State.RicordaHeard || State.CompletedFloor >= SpeakerFloor - 1);
    }

    inline int Revision(const Progress& State)
    {
        return State.CompletedFloor + (State.RicordaHeard ? 1 : 0);
    }

    inline constexpr bool MatchesDoubleClearingLatchProgress(std::uint8_t Step, int CompletedFloor)
    {
        if (Step > DoubleClearingLatchMaxStep) return false;
        if (CompletedFloor >= DoubleClearingFloor) return Step == DoubleClearingLatchMaxStep;
        if (CompletedFloor == DoubleClearingFloor - 1) return Step < DoubleClearingLatchMaxStep;
        return Step == 0;
    }

    inline std::optional<std::uint8_t> ProposeDoubleClearingLatchStep(std::uint8_t Committed, std::uint8_t Expected)
    {
        if (Committed != Expected || Committed >= DoubleClearingLatchMaxStep)
            return std::nullopt;
        return static_cast<std::uint8_t>(Committed + 1);
    }

    // Source and floor are authenticated by the runtime adapter before reducing.
    // Commit the returned state and any mission or memory effects in one save.
    inline std::optional<Progress> Advance(const Progress& State, const Event& Input)
    {
        if (!IsValid(State) || State.CompletedFloor == GuardianFloor
            || Input.ActiveChapter != Chapter || Input.ExpectedRevision != Revision(State))
        {
            return std::nullopt;
        }

        const int Floor = State.CompletedFloor + 1;
        if (Input.AuthenticatedFloor != Floor || Input.TargetFloor != Floor)
            return std::nullopt;

        if (Input.Kind == EventKind::RicordaHeard)
        {
            if (Floor == SpeakerFloor && !State.RicordaHeard
                && Input.Source == Producer::StorySequence && Input.Subject == Target::NamelessSpeaker)
            {
                return Progress{State.CompletedFloor, true};
            }
            return std::nullopt;
        }

        if (Floor == SpeakerFloor && !State.RicordaHeard)
            return std::nullopt;

        if (Floor == SluiceFloor || Floor == FernBasinFloor)
        {
            const bool FernBasin = Floor == FernBasinFloor;
            if (Input.Kind != (FernBasin ? EventKind::FernBasinDrained : EventKind::SluicesOpened) ||
                Input.Source != Producer::Puzzle ||
                Input.Subject != (FernBasin ? Target::FernBasinDrains : Target::SluiceControls))
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == MarrowFloor)
        {
            if (Input.Kind != EventKind::MarrowLatchOpened || Input.Source != Producer::Puzzle ||
                Input.Subject != Target::MarrowLatch)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == CounterweightFloor)
        {
            if (Input.Kind != EventKind::CounterweightInstalled || Input.Source != Producer::Puzzle ||
                Input.Subject != Target::CounterweightWinch)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == MirrorFloor)
        {
            if (Input.Kind != EventKind::MirrorsAligned || Input.Source != Producer::Puzzle ||
                Input.Subject != Target::MirrorControls)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == PincerWinchFloor)
        {
            if (Input.Kind != EventKind::PincerWinchOpened || Input.Source != Producer::Puzzle ||
                Input.Subject != Target::PincerWinch)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == BranchLatchFloor)
        {
            if (Input.Kind != EventKind::BranchLatchSelected || Input.Source != Producer::Puzzle ||
                Input.Subject != Target::BranchLatches)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == RootclawFloor)
        {
            if (Input.Kind != EventKind::RootclawDefeated || Input.Source != Producer::Encounter ||
                Input.Subject != Target::Rootclaw)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == ObliqueClearingFloor || Floor == VeinOverlookFloor)
        {
            if (Input.Kind != EventKind::FloorTraversed || Input.Source != Producer::Traversal ||
                Input.Subject != Target::TraversalExit)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == BranchTerraceFloor)
        {
            if (Input.Kind != EventKind::BranchStairReleased || Input.Source != Producer::Puzzle ||
                Input.Subject != Target::BranchStairRelease)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == HollowTrunkFloor || Floor == RootletCanalFloor)
        {
            const bool Rootlet = Floor == RootletCanalFloor;
            if (Input.Kind != (Rootlet ? EventKind::RootletCounterweightReleased : EventKind::HollowTrunkPinSeated)
                || Input.Source != Producer::Puzzle
                || Input.Subject != (Rootlet ? Target::RootletCounterweight : Target::HollowTrunkSocket))
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == SuspendedTrunksFloor)
        {
            if (Input.Kind != EventKind::SuspendedBridgeAligned || Input.Source != Producer::Puzzle ||
                Input.Subject != Target::SuspendedBridgeControl)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == DoubleClearingFloor)
        {
            if (Input.Kind != EventKind::DoubleClearingLatchOpened || Input.Source != Producer::Puzzle ||
                Input.Subject != Target::DoubleClearingLatch)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        if (Floor == RiftrootFloor)
        {
            if (Input.Kind != EventKind::RiftrootDefeated || Input.Source != Producer::Encounter
                || Input.Subject != Target::Riftroot)
                return std::nullopt;
            return Progress{Floor, State.RicordaHeard};
        }

        const bool Guardian = Floor == GuardianFloor;
        if (Input.Source != Producer::Encounter
            || Input.Kind != (Guardian ? EventKind::GuardianDefeated : EventKind::FloorCleared)
            || Input.Subject != (Guardian ? Target::AbyssGuardian : Target::FloorEncounter))
        {
            return std::nullopt;
        }

        return Progress{Floor, State.RicordaHeard};
    }
}
