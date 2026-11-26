#pragma once

#include <cmath>

namespace EidolonRootletCanal
{
    // Offsets are signed distances from the closed gate along its normal.
    inline bool IsOnReleaseSide(double PawnOffset, double ControlOffset)
    {
        constexpr double GateClearanceCm = 40.0;
        return std::isfinite(PawnOffset) && std::isfinite(ControlOffset)
            && std::abs(ControlOffset) > GateClearanceCm
            && (ControlOffset > 0.0 ? PawnOffset : -PawnOffset) > GateClearanceCm;
    }
}
