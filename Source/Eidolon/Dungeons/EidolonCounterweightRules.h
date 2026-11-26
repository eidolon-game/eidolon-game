#pragma once

#include <cstdint>
#include <limits>
#include <optional>

namespace EidolonCounterweight
{
    enum class Location { AtSource, Carried, Installed };
    enum class Action { Collect, Install, Recover };
    enum class Evidence { Source, Winch, CarrierRecovery };

    struct Progress
    {
        Location Weight = Location::AtSource;
        std::uint32_t Revision = 0;
    };

    struct Event
    {
        Progress Expected;
        Action Kind;
        Evidence VerifiedAt;
    };

    inline constexpr bool IsValid(const Progress& Committed)
    {
        const bool Odd = (Committed.Revision & 1u) != 0;
        switch (Committed.Weight)
        {
            case Location::AtSource: return !Odd;
            case Location::Carried: return Odd;
            case Location::Installed: return !Odd && Committed.Revision >= 2;
        }

        return false;
    }

    inline constexpr bool CanExit(const Progress& Committed)
    {
        return IsValid(Committed) && Committed.Weight == Location::Installed;
    }

    // Evidence is authenticated by the native source, winch or carrier adapter.
    // Publish a proposal only after saving the checkpoint containing that proposal.
    inline std::optional<Progress> Advance(const Progress& Committed, const Event& Input)
    {
        if (!IsValid(Committed) || Input.Expected.Weight != Committed.Weight
            || Input.Expected.Revision != Committed.Revision
            || Committed.Revision == std::numeric_limits<std::uint32_t>::max())
        {
            return std::nullopt;
        }

        const std::uint32_t NextRevision = Committed.Revision + 1;
        switch (Input.Kind)
        {
            case Action::Collect:
                if (Committed.Weight == Location::AtSource && Input.VerifiedAt == Evidence::Source)
                    return Progress{Location::Carried, NextRevision};
                break;
            case Action::Install:
                if (Committed.Weight == Location::Carried && Input.VerifiedAt == Evidence::Winch)
                    return Progress{Location::Installed, NextRevision};
                break;
            case Action::Recover:
                if (Committed.Weight == Location::Carried && Input.VerifiedAt == Evidence::CarrierRecovery)
                    return Progress{Location::AtSource, NextRevision};
                break;
        }

        return std::nullopt;
    }
}
