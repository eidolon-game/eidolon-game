#include "../Source/Eidolon/Quest/EidolonFirstDescentRules.h"

#include <iostream>

using namespace EidolonFirstDescent;

int main()
{
    constexpr std::array<std::string_view, 7> CanonicalOrder{{
        "guild_register", "abyss_floor_1", "tutorial_monster_defeated",
        "first_anomaly", "surface_return", "dormitory_night", "noah_reveal"
    }};

    std::size_t Cursor = 0;
    for (std::string_view CanonicalEvent : CanonicalOrder)
    {
        const Event Valid{2, "mq_02", Cursor, Steps[Cursor]};
        if (Valid.Payload.Event != CanonicalEvent)
        {
            std::cerr << "Unexpected event order at cursor " << Cursor << '\n';
            return 1;
        }

        const std::array<Event, 6> Invalid{{
            {3, Valid.Quest, Cursor, Valid.Payload},
            {2, "mq_03", Cursor, Valid.Payload},
            {2, Valid.Quest, Cursor + 1, Valid.Payload},
            {2, Valid.Quest, Cursor, {CanonicalEvent, Valid.Payload.Source, "wrong_target"}},
            {2, Valid.Quest, Cursor, {CanonicalEvent,
                Valid.Payload.Source == Producer::StorySequence ? Producer::LevelTravel : Producer::StorySequence,
                Valid.Payload.Target}},
            {2, Valid.Quest, Cursor, Steps[(Cursor + 1) % Steps.size()]}
        }};
        for (const Event& Rejected : Invalid)
        {
            if (Advance(Cursor, Rejected))
            {
                std::cerr << "Invalid event accepted at cursor " << Cursor << '\n';
                return 1;
            }
        }

        const auto Next = Advance(Cursor, Valid);
        if (!Next || *Next != Cursor + 1 || Advance(*Next, Valid))
        {
            std::cerr << "Transition or replay handling failed at cursor " << Cursor << '\n';
            return 1;
        }
        Cursor = *Next;
    }

    if (Cursor != Steps.size() ||
        Advance(Cursor, {2, "mq_02", Cursor, Steps.back()}))
    {
        std::cerr << "Completed sequence accepted another event\n";
        return 1;
    }

    std::cout << "First Descent: 7 ordered transitions, 50 rejected invalid or replayed events\n";
}
