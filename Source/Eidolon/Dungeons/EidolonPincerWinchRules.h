#pragma once

#include <cstdint>
#include <optional>

namespace EidolonPincerWinch
{
    enum class Stage : std::uint8_t
    {
        Initial,
        PincerDefeated,
        FirstWinchEngaged,
        CrawlerDefeated,
        Open
    };

    enum class Signal : std::uint8_t
    {
        ConfirmPincerDefeat,
        EngageFirstWinch,
        ConfirmCrawlerDefeat,
        EngageSecondWinch
    };

    inline constexpr bool IsValid(Stage Committed)
    {
        return static_cast<std::uint8_t>(Committed) <= static_cast<std::uint8_t>(Stage::Open);
    }

    inline constexpr bool CanExit(Stage Committed)
    {
        return Committed == Stage::Open;
    }

    // Encounter activation is transient. The native adapter confirms the active
    // enemy or exact winch, then saves the proposed stage before applying effects.
    inline std::optional<Stage> Propose(Stage Committed, Stage Expected, Signal Input)
    {
        if (Committed != Expected) return std::nullopt;

        switch (Committed)
        {
            case Stage::Initial:
                if (Input == Signal::ConfirmPincerDefeat) return Stage::PincerDefeated;
                break;
            case Stage::PincerDefeated:
                if (Input == Signal::EngageFirstWinch) return Stage::FirstWinchEngaged;
                break;
            case Stage::FirstWinchEngaged:
                if (Input == Signal::ConfirmCrawlerDefeat) return Stage::CrawlerDefeated;
                break;
            case Stage::CrawlerDefeated:
                if (Input == Signal::EngageSecondWinch) return Stage::Open;
                break;
            case Stage::Open:
                break;
        }

        return std::nullopt;
    }
}
