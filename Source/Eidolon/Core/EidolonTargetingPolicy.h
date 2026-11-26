#pragma once
#include "CoreMinimal.h"

namespace Eidolon::Targeting
{
    // Lower is better. Negative means ineligible. Distances are centimeters.
    inline double Score(double Distance, double Alignment, double Range, double MinAlignment)
    {
        if (!FMath::IsFinite(Distance) || !FMath::IsFinite(Alignment) ||
            !FMath::IsFinite(Range) || !FMath::IsFinite(MinAlignment) ||
            Range <= 0.0 || Distance <= UE_SMALL_NUMBER || Distance > Range ||
            Alignment < MinAlignment || Alignment > 1.0 || Alignment < -1.0)
            return -1.0;
        return Distance / Range + 2.0 * (1.0 - Alignment);
    }
}
