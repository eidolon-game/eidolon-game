#include "../Source/Eidolon/Dungeons/EidolonSluiceRules.h"
#include "../Source/Eidolon/Dungeons/EidolonAbyssProgression.h"

#include <iostream>

using namespace EidolonSluice;

int main()
{
    for (const Control First : {Control::Inlet, Control::Outlet})
    {
        const Control Second = First == Control::Inlet ? Control::Outlet : Control::Inlet;
        Progress Committed = 0;
        const auto FirstCandidate = ProposeActivation(Committed, Committed, First);
        if (CanExit(Committed) || !FirstCandidate || !IsActivated(*FirstCandidate, First)
            || IsActivated(*FirstCandidate, Second) || CanExit(*FirstCandidate))
        {
            std::cerr << "One lever must leave the exit locked\n";
            return 1;
        }

        Committed = *FirstCandidate; // Successful checkpoint.
        const Progress Resumed = Committed;
        if (!IsActivated(Resumed, First) || CanExit(Resumed)
            || ProposeActivation(Resumed, Resumed, First)
            || ProposeActivation(Resumed, 0, Second))
        {
            std::cerr << "Resumed progress, duplicate or stale event handling failed\n";
            return 1;
        }

        const auto RetryCandidate = ProposeActivation(Resumed, Resumed, Second);
        // The first write of this candidate fails. The committed state stays unchanged.
        if (!RetryCandidate || CanExit(Committed) || !IsActivated(Committed, First)
            || IsActivated(Committed, Second) || ProposeActivation(Committed, Committed, Second) != RetryCandidate)
        {
            std::cerr << "Failed checkpoint advanced or lost progress\n";
            return 1;
        }

        Committed = *RetryCandidate; // Retry checkpoint succeeds.
        if (!CanExit(Committed) || !IsActivated(Committed, First) || !IsActivated(Committed, Second)
            || ProposeActivation(Committed, Resumed, Second) || ProposeActivation(Committed, Committed, First))
        {
            std::cerr << "Both committed levers must open the exit permanently\n";
            return 1;
        }
    }

    for (unsigned Mask = 0; Mask <= 255; ++Mask)
    {
        const auto Saved = static_cast<Progress>(Mask);
        if (IsValid(Saved) != (Mask <= 3) || CanExit(Saved) != (Mask == 3)
            || IsActivated(Saved, static_cast<Control>(3))
            || ProposeActivation(Saved, Saved, static_cast<Control>(3))
            || (!IsValid(Saved) && ProposeActivation(Saved, Saved, Control::Inlet)))
        {
            std::cerr << "Invalid mask or lever accepted: " << Mask << '\n';
            return 1;
        }
    }

    for (const int Floor : {EidolonAbyss::SluiceFloor, EidolonAbyss::FernBasinFloor})
    {
        if (!MatchesFloorProgress(0, Floor - 2, Floor) || MatchesFloorProgress(1, Floor - 2, Floor) ||
            !MatchesFloorProgress(1, Floor - 1, Floor) || !MatchesFloorProgress(2, Floor - 1, Floor) ||
            MatchesFloorProgress(AllControls, Floor - 1, Floor) || MatchesFloorProgress(1, Floor, Floor) ||
            !MatchesFloorProgress(AllControls, Floor, Floor))
        {
            std::cerr << "Saved drain state is inconsistent with completion for floor " << Floor << '\n';
            return 1;
        }
    }

    std::cout << "Sluice: both orders, resume, failed-write retry, replay and all 256 masks passed\n";
}
