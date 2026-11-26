#include "../Source/Eidolon/Dungeons/EidolonMarrowLatchRules.h"

#include <cassert>
#include <iostream>

int main()
{
    using namespace EidolonMarrowLatch;
    assert(!Propose(Stage::Locked, Stage::Locked, Signal::OperateLatch));
    assert(!Propose(Stage::Locked, Stage::ReleasedByImpact, Signal::OperateLatch));

    for (const Signal Cause : {Signal::BeamImpact, Signal::RamDefeat})
    {
        auto Saved = Stage::Locked;
        const auto Release = Propose(Saved, Saved, Cause);
        assert(Release && IsReleased(*Release));
        assert(*Release == (Cause == Signal::BeamImpact ? Stage::ReleasedByImpact : Stage::ReleasedByDefeat));

        // A failed write returns the same proposal on retry and keeps the latch locked.
        assert(Saved == Stage::Locked);
        assert(Propose(Saved, Saved, Cause) == Release);
        Saved = *Release;
        assert(!Propose(Saved, Stage::Locked, Signal::OperateLatch));
        assert(!Propose(Saved, Saved, Signal::BeamImpact));
        assert(!Propose(Saved, Saved, Signal::RamDefeat));
        const auto Open = Propose(Saved, Saved, Signal::OperateLatch);
        assert(Open && *Open == Stage::Open);
        assert(!Propose(*Open, *Open, Signal::OperateLatch));
    }

    const auto UnknownStage = static_cast<Stage>(255);
    assert(!Propose(UnknownStage, UnknownStage, Signal::OperateLatch));
    assert(!Propose(Stage::Locked, Stage::Locked, static_cast<Signal>(255)));
    std::cout << "Marrow latch: distinct release routes, retry, stale revision and replay passed\n";
}
