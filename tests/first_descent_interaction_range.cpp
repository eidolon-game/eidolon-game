#include "../Source/Eidolon/Quest/EidolonInteractionRange.h"

#include <array>
#include <iostream>
#include <limits>

int main()
{
    constexpr double Infinity = std::numeric_limits<double>::infinity();
    constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
    struct Fixture
    {
        double DistanceSquared;
        double Range;
        bool Expected;
    };
    const std::array<Fixture, 12> Cases{{
        {0.0, 1.0, true},
        {22500.0, 150.0, true},
        {22501.0, 150.0, false},
        {44100.0, 200.0, false},
        {-1.0, 150.0, false},
        {Infinity, 150.0, false},
        {NaN, 150.0, false},
        {0.0, 0.0, false},
        {0.0, -1.0, false},
        {1.0, Infinity, false},
        {1.0, NaN, false},
        {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), true}
    }};

    for (std::size_t Index = 0; Index < Cases.size(); ++Index)
    {
        const auto& [DistanceSquared, Range, Expected] = Cases[Index];
        if (EidolonQuest::IsWithinInteractionRange(DistanceSquared, Range) != Expected)
        {
            std::cerr << "Interaction range fixture failed: " << Index << '\n';
            return 1;
        }
    }

    std::cout << "First Descent: " << Cases.size() << " interaction range cases passed\n";
}
