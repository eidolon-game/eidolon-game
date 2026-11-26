#pragma once

#include <cmath>

namespace EidolonQuest
{
    inline bool IsWithinInteractionRange(double DistanceSquared, double MaxDistanceCm)
    {
        return std::isfinite(DistanceSquared) && DistanceSquared >= 0.0
            && std::isfinite(MaxDistanceCm) && MaxDistanceCm > 0.0
            && std::sqrt(DistanceSquared) <= MaxDistanceCm;
    }
}
