#include "../Source/Eidolon/Dungeons/EidolonPincerWinchRules.h"

#include <array>
#include <iostream>

using namespace EidolonPincerWinch;

int main()
{
    constexpr std::array<Stage, 5> Sequence{{
        Stage::Initial, Stage::PincerDefeated, Stage::FirstWinchEngaged, Stage::CrawlerDefeated, Stage::Open
    }};
    constexpr std::array<Signal, 4> Signals{{
        Signal::ConfirmPincerDefeat, Signal::EngageFirstWinch, Signal::ConfirmCrawlerDefeat, Signal::EngageSecondWinch
    }};

    Stage Committed = Stage::Initial;
    for (std::size_t Index = 0; Index < Signals.size(); ++Index)
    {
        const Stage Resumed = Committed;
        const Signal Input = Signals[Index];
        const auto Candidate = Propose(Resumed, Resumed, Input);
        // A failed checkpoint leaves the committed stage where it was.
        if (CanExit(Committed) || !Candidate || *Candidate != Sequence[Index + 1]
            || Propose(Committed, Resumed, Input) != Candidate
            || Propose(*Candidate, Resumed, Input))
        {
            std::cerr << "Ordered checkpoint, simulated write retry or stage replay failed at " << Index << '\n';
            return 1;
        }
        Committed = *Candidate;
    }
    if (!CanExit(Committed))
    {
        std::cerr << "Completing both encounters and both winch actions did not open the gate\n";
        return 1;
    }

    for (unsigned Value = 0; Value <= 255; ++Value)
    {
        const auto StageValue = static_cast<Stage>(Value);
        if (IsValid(StageValue) != (Value < Sequence.size())
            || CanExit(StageValue) != (Value == Sequence.size() - 1))
        {
            std::cerr << "Malformed persisted stage accepted\n";
            return 1;
        }

        for (unsigned RawSignal = 0; RawSignal <= 255; ++RawSignal)
        {
            const auto Input = static_cast<Signal>(RawSignal);
            const auto Candidate = Propose(StageValue, StageValue, Input);
            const bool Allowed = Value < Signals.size() && Input == Signals[Value];
            if (Candidate.has_value() != Allowed ||
                (Candidate && *Candidate != Sequence[Value + 1]))
            {
                std::cerr << "Unexpected stage and signal pairing\n";
                return 1;
            }
        }
    }

    std::cout << "Pincer winch: ordered receipts, resume, retry, replay and 65536 stage/signal combinations passed\n";
}
