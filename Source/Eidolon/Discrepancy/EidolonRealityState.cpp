#include "EidolonRealityState.h"

namespace
{
    const FName Noah(TEXT("npc.noah")), Vareth(TEXT("city.vareth")), Survivor(TEXT("npc.vareth_survivor"));
    const FName Appear(TEXT("noah.appear")), Erase(TEXT("vareth.erase")), Enter(TEXT("oblivion.enter"));
    const FName Disappear(TEXT("noah.disappear")), Recover(TEXT("noah.recover"));
}
FEidolonRealityState FEidolonRealityState::Initial()
{
    FEidolonRealityState S;
    S.Entities.Add(Noah, EEidolonPresence::Absent);
    S.Entities.Add(Vareth, EEidolonPresence::Present);
    S.Entities.Add(Survivor, EEidolonPresence::Present);
    S.OriginalMemories.Add(TEXT("family.before_noah"));
    for (FName N : {FName(TEXT("npc.darius")), FName(TEXT("npc.elara")), FName(TEXT("npc.lyra")), FName(TEXT("npc.riven"))})
        S.CurrentMemories.Add(N, TEXT("family.before_noah"));
    return S;
}
bool FEidolonRealityState::IsValid() const
{
    if (Revision < 0 || Revision != AppliedEvents.Num() || !Entities.Contains(Noah) || !Entities.Contains(Vareth) || !Entities.Contains(Survivor)) return false;
    if (AppliedEvents.Contains(Recover) && !AppliedEvents.Contains(Disappear)) return false;
    if (AppliedEvents.Contains(Disappear) && !AppliedEvents.Contains(Appear)) return false;
    if (AppliedEvents.Contains(Erase) && Entities.FindRef(Vareth) != EEidolonPresence::Oblivion) return false;
    if (AppliedEvents.Contains(Recover) && Entities.FindRef(Noah) != EEidolonPresence::Oblivion && Ending.IsNone()) return false;
    return Reality == TEXT("prime") || Reality == TEXT("oblivion") || Reality == TEXT("alternate.valdris");
}
bool FEidolonRealityState::Apply(FName Event, int32 Chapter, int32 ExpectedRevision)
{
    if (!IsValid() || !Ending.IsNone() || ExpectedRevision != Revision || Revision == MAX_int32 || AppliedEvents.Contains(Event)) return false;
    auto Next = *this;
    auto FamilyMemory = [&Next](FName Memory) { for (auto& Pair : Next.CurrentMemories) Pair.Value = Memory; };
    if (Event == Appear && Chapter == 2 && Next.Entities.FindRef(Noah) == EEidolonPresence::Absent)
    {
        Next.Entities[Noah] = EEidolonPresence::Present;
        FamilyMemory(TEXT("family.with_noah"));
    }
    else if (Event == Erase && Chapter == 10 && Next.Entities.FindRef(Vareth) == EEidolonPresence::Present)
    {
        Next.Entities[Vareth] = EEidolonPresence::Oblivion;
        Next.Entities[Survivor] = EEidolonPresence::Oblivion;
        Next.OriginalMemories.Add(TEXT("vareth.survivor"));
    }
    else if (Event == Enter && Chapter == 20)
    {
        Next.Flags.Add(TEXT("oblivion.unlocked"));
        Next.Reality = TEXT("oblivion");
    }
    else if (Event == TEXT("alternate.family") && Chapter == 21 && Next.Flags.Contains(TEXT("oblivion.unlocked")))
    {
        Next.Reality = TEXT("alternate.valdris");
        Next.OriginalMemories.Add(TEXT("family.alternate"));
    }
    else if (Event == Disappear && Chapter == 22 && Next.AppliedEvents.Contains(Appear) && Next.Flags.Contains(TEXT("oblivion.unlocked")))
    {
        Next.Entities[Noah] = EEidolonPresence::Oblivion;
        Next.Reality = TEXT("prime");
        Next.OriginalMemories.Add(TEXT("family.with_noah"));
        FamilyMemory(TEXT("family.before_noah"));
    }
    else if (Event == Recover && Chapter == 22 && Next.AppliedEvents.Contains(Disappear))
    {
        Next.Reality = TEXT("oblivion");
        Next.Flags.Add(TEXT("noah_found_in_oblivion"));
    }
    else return false;
    Next.AppliedEvents.Add(Event);
    ++Next.Revision;
    if (!Next.IsValid()) return false;
    *this = MoveTemp(Next);
    return true;
}
bool FEidolonRealityState::SelectEnding(FName Choice, int32 Chapter, bool bBossDefeated, bool bLostMemories, bool bAid, bool bTruth)
{
    if (!IsValid() || !Ending.IsNone() || Chapter != 26 || !bBossDefeated || !AppliedEvents.Contains(Recover) || Revision == MAX_int32) return false;
    if (Choice != TEXT("custodian") && Choice != TEXT("origin") && Choice != TEXT("guild") && Choice != TEXT("freedom")) return false;
    if (Choice == TEXT("freedom") && !(bLostMemories && bAid && bTruth)) return false;
    auto Next = *this;
    Next.Ending = Choice;
    Next.Reality = TEXT("prime");
    Next.Entities[Noah] = Choice == TEXT("origin") ? EEidolonPresence::Absent : EEidolonPresence::Present;
    for (auto& Pair : Next.CurrentMemories) Pair.Value = Choice == TEXT("origin") ? FName(TEXT("family.before_noah")) : FName(TEXT("family.with_noah"));
    if (Choice == TEXT("freedom")) Next.Flags.Add(TEXT("reality.stable_passages"));
    Next.AppliedEvents.Add(TEXT("ending.selected"));
    ++Next.Revision;
    if (!Next.IsValid()) return false;
    *this = MoveTemp(Next);
    return true;
}
