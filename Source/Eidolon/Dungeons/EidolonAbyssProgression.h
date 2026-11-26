#pragma once

#include <optional>

namespace EidolonAbyss
{
    inline constexpr int Chapter = 4;
    inline constexpr int SluiceFloor = 4;
    inline constexpr int SpeakerFloor = 90;
    inline constexpr int GuardianFloor = 100;

    enum class EventKind
    {
        FloorCleared,
        SluicesOpened,
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
