#pragma once

#include <cstdint>
#include <optional>

namespace EidolonMarrowLatch
{
    enum class Stage : std::uint8_t { Locked, ReleasedByImpact, ReleasedByDefeat, Open };
    enum class Signal : std::uint8_t { BeamImpact, RamDefeat, OperateLatch };

    inline bool IsValid(Stage Committed)
    {
        return Committed == Stage::Locked || Committed == Stage::ReleasedByImpact ||
            Committed == Stage::ReleasedByDefeat || Committed == Stage::Open;
    }

    inline bool IsReleased(Stage Committed)
    {
        return Committed == Stage::ReleasedByImpact || Committed == Stage::ReleasedByDefeat;
    }

    // The floor validates the tracked Ram or latch first. Apply the proposal only after writing a checkpoint.
    inline std::optional<Stage> Propose(Stage Committed, Stage Expected, Signal Verified)
    {
        if (Committed != Expected) return std::nullopt;
        if (Committed == Stage::Locked)
        {
            if (Verified == Signal::BeamImpact) return Stage::ReleasedByImpact;
            if (Verified == Signal::RamDefeat) return Stage::ReleasedByDefeat;
        }
        if (IsReleased(Committed) && Verified == Signal::OperateLatch) return Stage::Open;
        return std::nullopt;
    }
}
