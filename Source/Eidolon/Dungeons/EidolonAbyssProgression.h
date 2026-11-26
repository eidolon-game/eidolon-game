#pragma once

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
        RicordaHeard,
        GuardianDefeated
    };

    enum class Producer
    {
        Encounter,
        Puzzle,
        StorySequence
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
        NamelessSpeaker,
        AbyssGuardian
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

        if (Floor == SluiceFloor)
        {
            if (Input.Kind != EventKind::SluicesOpened || Input.Source != Producer::Puzzle ||
                Input.Subject != Target::SluiceControls)
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
