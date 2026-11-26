#pragma once

#include <cstdint>
#include <optional>

namespace EidolonBranchLatch
{
    enum class Route : std::uint8_t
    {
        Unselected,
        First,
        Second
    };

    inline constexpr bool CanExit(Route Committed)
    {
        return Committed == Route::First || Committed == Route::Second;
    }

    inline constexpr bool IsValid(Route Committed)
    {
        return Committed == Route::Unselected || CanExit(Committed);
    }

    // The native adapter determines the route from the actual registered latch.
    // Change the environment only after the route and completion have saved.
    inline std::optional<Route> Propose(Route Committed, Route Expected, Route VerifiedLatch)
    {
        if (Committed != Expected || Committed != Route::Unselected || !CanExit(VerifiedLatch))
            return std::nullopt;

        return VerifiedLatch;
    }
}
