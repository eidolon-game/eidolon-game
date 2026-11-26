#include "../Source/Eidolon/Dungeons/EidolonMirrorRules.h"

#include <iostream>

using namespace EidolonMirror;

int main()
{
    Progress Committed;
    const Event FirstTurn{Committed, Control::First};
    const auto First = Advance(Committed, FirstTurn);
    if (!First || First->First != FirstGoal || First->Second != 0 || First->Revision != 1
        || CanExit(*First) || Advance(*First, FirstTurn)
        || Advance(Committed, {Committed, static_cast<Control>(255)}))
    {
        std::cerr << "First mirror did not rotate independently or rejected a valid alignment\n";
        return 1;
    }
    Committed = *First;

    const Progress Resumed = Committed;
    const auto Second = Advance(Resumed, {Resumed, Control::Second});
    if (!Second || Second->First != FirstGoal || Second->Second != 1 || CanExit(*Second))
    {
        std::cerr << "Resuming lost first alignment or opened the gate early\n";
        return 1;
    }
    Committed = *Second;

    const Event Finish{Committed, Control::Second};
    const auto Aligned = Advance(Committed, Finish);
    if (!Aligned || CanExit(Committed) || !CanExit(*Aligned) || Aligned->Revision != 3
        || !Advance(Committed, Finish))
    {
        std::cerr << "A failed save could not retry the same candidate\n";
        return 1;
    }
    Committed = *Aligned;
    if (Advance(Committed, Finish) || Advance(Committed, {Committed, Control::First}) ||
        Advance(Committed, {Committed, Control::Second}))
    {
        std::cerr << "Completed puzzle allowed a later control to reverse the alignment\n";
        return 1;
    }

    Progress Wrapped;
    for (int Turn = 0; Turn < OrientationCount; ++Turn)
    {
        const auto Next = Advance(Wrapped, {Wrapped, Control::Second});
        if (!Next || Next->First != 0 || Next->Second != (Turn + 1) % OrientationCount || CanExit(*Next))
        {
            std::cerr << "Four-orientation wrap failed or matched without the first mirror\n";
            return 1;
        }
        Wrapped = *Next;
    }
    if (Advance(Wrapped, FirstTurn))
    {
        std::cerr << "A stale event was accepted after both orientations returned to zero\n";
        return 1;
    }

    const auto SecondAligned = Advance(Progress{0, 1, 1}, {{0, 1, 1}, Control::Second});
    if (!SecondAligned || CanExit(*SecondAligned))
    {
        std::cerr << "Second target alone opened the gate\n";
        return 1;
    }
    const auto BothReversed = Advance(*SecondAligned, {*SecondAligned, Control::First});
    const Progress Overflow{3, 0, std::numeric_limits<std::uint32_t>::max()};
    if (!BothReversed || !CanExit(*BothReversed) || !IsValid(Overflow)
        || Advance(Overflow, {Overflow, Control::Second})
        || IsValid({4, 0, 4}) || IsValid({3, 3, 2}) || IsValid({1, 2, 2})
        || CanExit({FirstGoal, SecondGoal, 2}))
    {
        std::cerr << "Reverse order, malformed persisted state or overflow handling failed\n";
        return 1;
    }

    std::cout << "Mirrors: independent four-way controls, both orders, resume, retry and replay passed\n";
}
