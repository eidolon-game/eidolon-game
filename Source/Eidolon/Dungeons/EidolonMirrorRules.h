#pragma once

#include <cstdint>
#include <limits>
#include <optional>

namespace EidolonMirror
{
    enum class Control { First, Second };
    inline constexpr std::uint8_t OrientationCount = 4;
    inline constexpr std::uint8_t FirstGoal = 1;
    inline constexpr std::uint8_t SecondGoal = 2;

    struct Progress
    {
        std::uint8_t First = 0;
        std::uint8_t Second = 0;
        std::uint32_t Revision = 0;
    };

    struct Event
    {
        Progress Expected;
        Control VerifiedMirror;
    };

    inline constexpr bool IsValid(const Progress& Committed)
    {
        const auto TotalTurns = static_cast<std::uint32_t>(Committed.First) + Committed.Second;
        return Committed.First < OrientationCount && Committed.Second < OrientationCount
            && Committed.Revision >= TotalTurns
            && Committed.Revision % OrientationCount == TotalTurns % OrientationCount;
    }

    inline constexpr bool CanExit(const Progress& Committed)
    {
        return IsValid(Committed) && Committed.First == FirstGoal && Committed.Second == SecondGoal;
    }

    // The native adapter verifies the actual mirror before proposing a turn.
    // Apply visual and gate changes only after the candidate is saved.
    inline std::optional<Progress> Advance(const Progress& Committed, const Event& Input)
    {
        if (!IsValid(Committed) || CanExit(Committed)
            || Input.Expected.First != Committed.First || Input.Expected.Second != Committed.Second
            || Input.Expected.Revision != Committed.Revision
            || Committed.Revision == std::numeric_limits<std::uint32_t>::max())
        {
            return std::nullopt;
        }

        Progress Next = Committed;
        switch (Input.VerifiedMirror)
        {
            case Control::First:
                Next.First = static_cast<std::uint8_t>((Next.First + 1) % OrientationCount);
                break;
            case Control::Second:
                Next.Second = static_cast<std::uint8_t>((Next.Second + 1) % OrientationCount);
                break;
            default:
                return std::nullopt;
        }

        ++Next.Revision;
        return Next;
    }
}
