#pragma once
#include "EidolonRuntimeContract.h"

// Shape compatible with department 08 FEvent without requiring its unmerged header.
struct FEidolonQuestEventFields
{
    FName Id, Type, Source, Target, Quest;
    int32 Chapter = 0;
};

inline bool EidolonProjectQuestEvent(const FEidolonRuntimeEvent& Event, FEidolonQuestEventFields& Out)
{
    if (!Event.IsValid() || Event.Domain != EEidolonEventDomain::Campaign ||
        Event.Kind != EEidolonEventKind::QuestObjective) return false;
    FEidolonQuestEventFields Next;
    Next.Id = FName(*Event.EventId.ToString(EGuidFormats::Digits));
    Next.Type = FName(*Event.SemanticId);
    Next.Source = FName(*Event.SourceId);
    Next.Target = FName(*Event.TargetId);
    Next.Quest = FName(*Event.QuestId);
    Next.Chapter = Event.Chapter;
    Out = Next;
    return true;
}
