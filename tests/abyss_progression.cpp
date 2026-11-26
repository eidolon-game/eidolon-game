#include "../Source/Eidolon/Dungeons/EidolonAbyssProgression.h"
#include "../Source/Eidolon/Dungeons/EidolonRootletCanalRules.h"

#include <iostream>
#include <limits>

using namespace EidolonAbyss;

int main()
{
    Progress State;
    for (int Floor = 1; Floor <= GuardianFloor; ++Floor)
    {
        const bool Guardian = Floor == GuardianFloor;
        Event Clear{Chapter, Floor, Floor, Revision(State), EventKind::FloorCleared,
            Producer::Encounter, Target::FloorEncounter};
        switch (Floor)
        {
            case SluiceFloor:
                Clear.Kind = EventKind::SluicesOpened;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::SluiceControls;
                break;
            case MarrowFloor:
                Clear.Kind = EventKind::MarrowLatchOpened;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::MarrowLatch;
                break;
            case CounterweightFloor:
                Clear.Kind = EventKind::CounterweightInstalled;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::CounterweightWinch;
                break;
            case MirrorFloor:
                Clear.Kind = EventKind::MirrorsAligned;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::MirrorControls;
                break;
            case PincerWinchFloor:
                Clear.Kind = EventKind::PincerWinchOpened;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::PincerWinch;
                break;
            case BranchLatchFloor:
                Clear.Kind = EventKind::BranchLatchSelected;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::BranchLatches;
                break;
            case GuardianFloor:
                Clear.Kind = EventKind::GuardianDefeated;
                Clear.Subject = Target::AbyssGuardian;
                break;
            case RootclawFloor:
                Clear.Kind = EventKind::RootclawDefeated;
                Clear.Subject = Target::Rootclaw;
                break;
            case ObliqueClearingFloor:
                Clear.Kind = EventKind::FloorTraversed;
                Clear.Source = Producer::Traversal;
                Clear.Subject = Target::TraversalExit;
                break;
            case BranchTerraceFloor:
                Clear.Kind = EventKind::BranchStairReleased;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::BranchStairRelease;
                break;
            case FernBasinFloor:
                Clear.Kind = EventKind::FernBasinDrained;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::FernBasinDrains;
                break;
            case HollowTrunkFloor:
                Clear.Kind = EventKind::HollowTrunkPinSeated;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::HollowTrunkSocket;
                break;
            case RootletCanalFloor:
                Clear.Kind = EventKind::RootletCounterweightReleased;
                Clear.Source = Producer::Puzzle;
                Clear.Subject = Target::RootletCounterweight;
                break;
        }
        const Event Rejected[]{
            {Chapter, Floor + 1, Floor + 1, Revision(State), Clear.Kind, Clear.Source, Clear.Subject},
            {Chapter, Floor - 1, Floor, Revision(State), Clear.Kind, Clear.Source, Clear.Subject},
            {Chapter, Floor, Floor + 1, Revision(State), Clear.Kind, Clear.Source, Clear.Subject},
            {Chapter, Floor, Floor, Revision(State) - 1, Clear.Kind, Clear.Source, Clear.Subject},
            {Chapter + 1, Floor, Floor, Revision(State), Clear.Kind, Clear.Source, Clear.Subject},
            {Chapter, Floor, Floor, Revision(State), Clear.Kind, Producer::StorySequence, Clear.Subject}
        };
        for (const Event& Invalid : Rejected)
        {
            if (Advance(State, Invalid))
            {
                std::cerr << "Invalid encounter allowed at floor " << Floor << '\n';
                return 1;
            }
        }

        const Event Memory{Chapter, Floor, Floor, Revision(State), EventKind::RicordaHeard,
            Producer::StorySequence, Target::NamelessSpeaker};
        const auto MemoryProgress = Advance(State, Memory);
        if (Floor == SpeakerFloor)
        {
            Event Impostor = Memory;
            Impostor.Subject = Target::AbyssGuardian;
            if (Advance(State, Impostor) || Advance(State, Clear) || !MemoryProgress
                || !MemoryProgress->RicordaHeard || MemoryProgress->CompletedFloor != Floor - 1
                || Advance(*MemoryProgress, Memory))
            {
                std::cerr << "Floor 90 speaker, ordering or replay failed\n";
                return 1;
            }
            State = *MemoryProgress;
            Clear.ExpectedRevision = Revision(State);
        }
        else if (MemoryProgress)
        {
            std::cerr << "Ricorda accepted off floor 90\n";
            return 1;
        }

        const Event Traverse{Chapter, Floor, Floor, Revision(State), EventKind::FloorTraversed,
            Producer::Traversal, Target::TraversalExit};
        if (Floor == ObliqueClearingFloor)
        {
            const Event Encounter{Chapter, Floor, Floor, Revision(State), EventKind::FloorCleared,
                Producer::Encounter, Target::FloorEncounter};
            Event WrongAnchor = Traverse;
            WrongAnchor.Subject = Target::FloorEncounter;
            Event WrongSource = Traverse;
            WrongSource.Source = Producer::Encounter;
            if (Advance(State, Encounter) || Advance(State, WrongAnchor) || Advance(State, WrongSource))
            {
                std::cerr << "The Oblique Clearing allowed a substitute for reaching its exit\n";
                return 1;
            }
        }
        else if (Advance(State, Traverse))
        {
            std::cerr << "Traversal receipt allowed on the wrong floor\n";
            return 1;
        }

        const Event WrongStair{Chapter, Floor, Floor, Revision(State), EventKind::BranchStairReleased,
            Producer::Puzzle, Floor == BranchTerraceFloor ? Target::BranchLatches : Target::BranchStairRelease};
        if (Advance(State, WrongStair))
        {
            std::cerr << "The Branch Terraces stair release was accepted at a different anchor or floor\n";
            return 1;
        }

        const Event Pin{Chapter, Floor, Floor, Revision(State), EventKind::HollowTrunkPinSeated,
            Producer::Puzzle, Target::HollowTrunkSocket};
        if (Floor == HollowTrunkFloor)
        {
            Event WrongSocket = Pin;
            WrongSocket.Subject = Target::CounterweightWinch;
            Event WrongProducer = Pin;
            WrongProducer.Source = Producer::Traversal;
            if (Advance(State, WrongSocket) || Advance(State, WrongProducer))
            {
                std::cerr << "The Hollow Trunk pin accepted a different socket or producer\n";
                return 1;
            }
        }
        else if (Advance(State, Pin))
        {
            std::cerr << "The Hollow Trunk pin was accepted outside floor 15\n";
            return 1;
        }

        const Event RootletWeight{Chapter, Floor, Floor, Revision(State), EventKind::RootletCounterweightReleased,
            Producer::Puzzle, Target::RootletCounterweight};
        if (Floor == RootletCanalFloor)
        {
            Event WrongWeight = RootletWeight;
            WrongWeight.Subject = Target::CounterweightWinch;
            Event WrongProducer = RootletWeight;
            WrongProducer.Source = Producer::Traversal;
            if (Advance(State, WrongWeight) || Advance(State, WrongProducer))
            {
                std::cerr << "The Rootlet Canal accepted a substitute counterweight or producer\n";
                return 1;
            }
        }
        else if (Advance(State, RootletWeight))
        {
            std::cerr << "The Rootlet counterweight was accepted outside floor 16\n";
            return 1;
        }

        if (Floor == FernBasinFloor)
        {
            Event CanalClear = Clear;
            CanalClear.Kind = EventKind::SluicesOpened;
            CanalClear.Subject = Target::SluiceControls;
            if (Advance(State, CanalClear))
            {
                std::cerr << "The Fern Basin accepted a Drainage Canals receipt\n";
                return 1;
            }
        }

        if (Guardian || Floor == RootclawFloor)
        {
            Event OrdinaryClear = Clear;
            OrdinaryClear.Kind = EventKind::FloorCleared;
            OrdinaryClear.Subject = Target::FloorEncounter;
            Event WrongTarget = Clear;
            WrongTarget.Subject = Target::NamelessSpeaker;
            if (Advance(State, OrdinaryClear) || Advance(State, WrongTarget))
            {
                std::cerr << "A capstone floor allowed a substitute for its boss\n";
                return 1;
            }
        }
        if (Clear.Source == Producer::Puzzle)
        {
            const Event Kill{Chapter, Floor, Floor, Revision(State),
                EventKind::FloorCleared, Producer::Encounter, Target::FloorEncounter};
            if (Advance(State, Kill))
            {
                std::cerr << "Killing an enemy cannot finish puzzle floor " << Floor << '\n';
                return 1;
            }
        }

        const auto Next = Advance(State, Clear);
        if (!Next || Next->CompletedFloor != Floor || Advance(*Next, Clear))
        {
            std::cerr << "Floor clear or replay failed at floor " << Floor << '\n';
            return 1;
        }
        State = *Next;
    }

    const Event OutOfBounds{Chapter, 101, 101, Revision(State),
        EventKind::FloorCleared, Producer::Encounter, Target::FloorEncounter};
    if (!IsValid(State) || !State.RicordaHeard || Advance(State, OutOfBounds)
        || IsValid({-1, false}) || IsValid({101, true})
        || !IsValid({89, false}) || IsValid({90, false}) || IsValid({88, true}))
    {
        std::cerr << "Abyss invalid-save or completion bounds failed\n";
        return 1;
    }

    using EidolonRootletCanal::IsOnReleaseSide;
    if (!IsOnReleaseSide(500.0, 500.0) || !IsOnReleaseSide(-500.0, -500.0) ||
        IsOnReleaseSide(-500.0, 500.0) || IsOnReleaseSide(500.0, -500.0) ||
        IsOnReleaseSide(40.0, 500.0) || IsOnReleaseSide(500.0, 0.0) ||
        IsOnReleaseSide(std::numeric_limits<double>::quiet_NaN(), 500.0))
    {
        std::cerr << "Rootlet Canal accepted a release on the wrong side of the gate\n";
        return 1;
    }

    std::cout << "Abyss: all 100 ordered clears plus floor 90 memory; invalid authority, skip, replay and bounds rejected\n";
}
