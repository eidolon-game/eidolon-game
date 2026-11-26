#pragma once

#include "CoreMinimal.h"

namespace EidolonMelee
{
    constexpr float MaxHeightDifference = 180.f;

    inline bool IsWithinReach(const FVector& Offset, float Reach)
    {
        return Offset.SizeSquared2D() <= FMath::Square(Reach)
            && FMath::Abs(Offset.Z) < MaxHeightDifference;
    }
}
