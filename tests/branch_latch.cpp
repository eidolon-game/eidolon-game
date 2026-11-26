#include "../Source/Eidolon/Dungeons/EidolonBranchLatchRules.h"

#include <iostream>

using namespace EidolonBranchLatch;

int main()
{
    for (const Route Choice : {Route::First, Route::Second})
    {
        const Route Other = Choice == Route::First ? Route::Second : Route::First;
        Route Committed = Route::Unselected;
        const auto Candidate = Propose(Committed, Committed, Choice);
        // An unsuccessful save leaves both choices available.
        if (CanExit(Committed) || !Candidate || *Candidate != Choice
            || Propose(Committed, Committed, Choice) != Candidate
            || !Propose(Committed, Committed, Other)
            || Propose(Committed, Choice, Choice))
        {
            std::cerr << "Selection may not publish before a successful save\n";
            return 1;
        }

        Committed = *Candidate;
        const Route Reloaded = Committed;
        if (!CanExit(Reloaded) || Propose(Reloaded, Route::Unselected, Choice)
            || Propose(Reloaded, Reloaded, Choice) || Propose(Reloaded, Reloaded, Other))
        {
            std::cerr << "One verified latch must persist without replay or switching\n";
            return 1;
        }
    }

    for (unsigned Current = 0; Current <= 255; ++Current)
    {
        const auto Committed = static_cast<Route>(Current);
        if (IsValid(Committed) != (Current <= 2) || CanExit(Committed) != (Current == 1 || Current == 2))
        {
            std::cerr << "Malformed route accepted\n";
            return 1;
        }
        for (unsigned Submitted = 0; Submitted <= 255; ++Submitted)
        {
            const auto Candidate = Propose(Committed, Committed, static_cast<Route>(Submitted));
            const bool Allowed = Current == 0 && (Submitted == 1 || Submitted == 2);
            if (Candidate.has_value() != Allowed || (Candidate && *Candidate != static_cast<Route>(Submitted)))
            {
                std::cerr << "Unexpected latch transition\n";
                return 1;
            }
        }
    }

    std::cout << "Branch latch: either verified route, retry, reload, replay and 65536 combinations passed\n";
}
