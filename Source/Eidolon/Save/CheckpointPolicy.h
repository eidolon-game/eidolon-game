#pragma once
#include <cstdint>
#include <limits>

namespace EidolonCheckpoint
{
    enum class Kind { Missing, Corrupt, Valid, Future, Unreadable };
    struct Copy { Kind kind; std::int64_t generation = -1; };
    struct Selection { int newest = -1; bool blocked = false; };
    inline Selection Select(Copy a, Copy b)
    {
        if (a.kind == Kind::Future || b.kind == Kind::Future ||
            a.kind == Kind::Unreadable || b.kind == Kind::Unreadable) return {-1, true};
        if ((a.kind == Kind::Valid && (a.generation < 0 || a.generation % 2 != 0)) ||
            (b.kind == Kind::Valid && (b.generation < 0 || b.generation % 2 != 1))) return {-1, true};
        if (a.kind == Kind::Valid && b.kind == Kind::Valid)
            return {a.generation > b.generation ? 0 : 1, false};
        if (a.kind == Kind::Valid) return {0, false};
        if (b.kind == Kind::Valid) return {1, false};
        return {-1, a.kind != Kind::Missing || b.kind != Kind::Missing};
    }
    struct Plan { int target = -1; std::int64_t generation = -1; };
    inline Plan Next(Copy a, Copy b, std::int64_t expected)
    {
        const auto s = Select(a,b);
        if (s.blocked) return {};
        const auto current = s.newest == 0 ? a.generation : s.newest == 1 ? b.generation : -1;
        if (expected != current || current == (std::numeric_limits<std::int64_t>::max)()) return {};
        const auto generation = current + 1;
        return {static_cast<int>(generation % 2), generation};
    }
}
