#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace EidolonFirstDescent
{
    enum class Producer
    {
        GuildRegistration,
        LevelTravel,
        Encounter,
        StorySequence
    };

    struct Step
    {
        std::string_view Event;
        Producer Source;
        std::string_view Target;
    };

    // Targets identify content bindings for the runtime adapter. It must authenticate
    // the producer and validate the actual actor or sequence before creating an event.
    inline constexpr std::array<Step, 7> Steps{{
        {"guild_register", Producer::GuildRegistration, "valdris_guild"},
        {"abyss_floor_1", Producer::LevelTravel, "abyss_001"},
        {"tutorial_monster_defeated", Producer::Encounter, "tutorial_arena"},
        {"first_anomaly", Producer::StorySequence, "anomaly"},
        {"surface_return", Producer::LevelTravel, "valdris_abyss"},
        {"dormitory_night", Producer::StorySequence, "guild_dormitory"},
        {"noah_reveal", Producer::StorySequence, "veyr_house"}
    }};

    struct Event
    {
        int Chapter;
        std::string_view Quest;
        std::size_t ExpectedCursor;
        Step Payload;
    };

    // Propose a new cursor. The caller must commit it atomically with the
    // accepted event receipt and its effects against the original cursor.
    inline std::optional<std::size_t> Advance(std::size_t Cursor, const Event& Input)
    {
        if (Input.Chapter != 2 || Input.Quest != "mq_02" ||
            Input.ExpectedCursor != Cursor || Cursor >= Steps.size())
        {
            return std::nullopt;
        }

        const Step& Expected = Steps[Cursor];
        if (Input.Payload.Event != Expected.Event ||
            Input.Payload.Source != Expected.Source ||
            Input.Payload.Target != Expected.Target)
        {
            return std::nullopt;
        }

        return Cursor + 1;
    }
}
