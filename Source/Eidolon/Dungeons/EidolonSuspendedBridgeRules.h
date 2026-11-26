#pragma once

#include <cmath>
#include <optional>

namespace EidolonSuspendedBridge
{
    inline std::optional<double> QuarterTurnDelta(double ClosedYaw, double OpenYaw)
    {
        if (!std::isfinite(ClosedYaw) || !std::isfinite(OpenYaw)) return std::nullopt;
        const double Delta = std::remainder(OpenYaw - ClosedYaw, 360.0);
        if (!(std::abs(std::abs(Delta) - 90.0) <= 1.0)) return std::nullopt;
        return Delta;
    }
}
