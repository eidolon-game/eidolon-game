#pragma once
#include "CoreMinimal.h"

namespace EidolonQuest
{
    // Pure rules; the Save department owns durable snapshots and atomic commits.
    struct FObjective
    {
        FName Id;
        FName Event;
        int32 Required = 1;
    };
    struct FEvent
    {
        FName Id;
        FName Type;
        FName Source;
        FName Target;
        FName Quest;
        int32 Chapter = 0;
    };
    struct FProgress
    {
        int32 Cursor = 0;
        int32 Count = 0;
        TSet<FName> Seen;
    };
    inline bool Apply(const TArray<FObjective>& Steps, FName Quest, int32 Chapter,
                      const FEvent& Event, FProgress& State)
    {
        if (Event.Id.IsNone() || Event.Source.IsNone() || Event.Target.IsNone() ||
            Quest.IsNone() || Event.Quest != Quest || Event.Chapter != Chapter ||
            State.Seen.Contains(Event.Id) || !Steps.IsValidIndex(State.Cursor)) return false;
        const auto& Step = Steps[State.Cursor];
        if (Step.Event.IsNone() || Step.Event != Event.Type || Step.Required < 1 ||
            State.Count < 0 || State.Count >= Step.Required) return false;
        State.Seen.Add(Event.Id);
        if (++State.Count == Step.Required) { ++State.Cursor; State.Count = 0; }
        return true;
    }
    inline bool IsComplete(const TArray<FObjective>& Steps, const FProgress& State)
    {
        return !Steps.IsEmpty() && State.Cursor == Steps.Num() && State.Count == 0;
    }
}
