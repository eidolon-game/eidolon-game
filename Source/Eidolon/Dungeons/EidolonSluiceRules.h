#pragma once

#include <cstdint>
#include <optional>

namespace EidolonSluice
{
    enum class Control : std::uint8_t
    {
        Inlet = 1,
        Outlet = 2
    };

    using Progress = std::uint8_t;
    inline constexpr Progress AllControls =
        static_cast<Progress>(Control::Inlet) | static_cast<Progress>(Control::Outlet);

    inline constexpr bool IsValid(Progress Committed)
    {
        return (Committed & AllControls) == Committed;
    }

    inline constexpr bool IsActivated(Progress Committed, Control Lever)
    {
        return IsValid(Committed) && (Lever == Control::Inlet || Lever == Control::Outlet)
            && (Committed & static_cast<Progress>(Lever)) != 0;
    }

    inline constexpr bool CanExit(Progress Committed)
    {
        return Committed == AllControls;
    }

    // The native adapter confirms the actual lever. Publish the candidate only
    // after the campaign checkpoint has saved successfully.
    inline std::optional<Progress> ProposeActivation(Progress Committed, Progress Expected, Control Lever)
    {
        if (!IsValid(Committed) || Expected != Committed ||
            (Lever != Control::Inlet && Lever != Control::Outlet) || IsActivated(Committed, Lever))
        {
            return std::nullopt;
        }

        return static_cast<Progress>(Committed | static_cast<Progress>(Lever));
    }
}
