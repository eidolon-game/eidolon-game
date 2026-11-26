#include "EidolonRealityState.h"

namespace
{
    const FName Noah(TEXT("npc_noah")), Vareth(TEXT("city_vareth")), Survivor(TEXT("npc_vareth_survivor"));
    const FName Appear(TEXT("noah_appear")), Erase(TEXT("vareth_erase")), Enter(TEXT("oblivion_enter"));
    const FName Disappear(TEXT("noah_disappear")), Recover(TEXT("noah_recover"));
}
FEidolonRealityState FEidolonRealityState::Initial()
{
    FEidolonRealityState S;
    S.Entities.Add(Noah, EEidolonPresence::Absent);
    S.Entities.Add(Vareth, EEidolonPresence::Present);
    S.Entities.Add(Survivor, EEidolonPresence::Present);
    S.OriginalMemories.Add(TEXT("family_before_noah"));
    for (FName N : {FName(TEXT("npc_darius")), FName(TEXT("npc_elara")), FName(TEXT("npc_lyra")), FName(TEXT("npc_riven"))})
        S.CurrentMemories.Add(N, TEXT("family_before_noah"));
    return S;
}
bool FEidolonRealityState::IsValid() const
{
    if (Revision < 0 || Revision != AppliedEvents.Num() || !Entities.Contains(Noah) || !Entities.Contains(Vareth) || !Entities.Contains(Survivor)) return false;
    if (!OriginalMemories.Contains(TEXT("family_before_noah"))) return false;
    if (AppliedEvents.Contains(Erase) && !AppliedEvents.Contains(Appear)) return false;
    if (AppliedEvents.Contains(Enter) && !AppliedEvents.Contains(Erase)) return false;
    if (AppliedEvents.Contains(TEXT("alternate_family")) && !AppliedEvents.Contains(Enter)) return false;
    if (AppliedEvents.Contains(Disappear) && !AppliedEvents.Contains(TEXT("alternate_family"))) return false;
    if (Ending.IsNone() != !AppliedEvents.Contains(TEXT("ending_selected"))) return false;
    const auto ExpectedNoah = !Ending.IsNone()
        ? (Ending == TEXT("origin") ? EEidolonPresence::Absent : EEidolonPresence::Present)
        : (AppliedEvents.Contains(Disappear) ? EEidolonPresence::Oblivion : (AppliedEvents.Contains(Appear) ? EEidolonPresence::Present : EEidolonPresence::Absent));
    if (Entities.FindRef(Noah) != ExpectedNoah) return false;
    if (AppliedEvents.Contains(Recover) && !AppliedEvents.Contains(Disappear)) return false;
    if (AppliedEvents.Contains(Disappear) && !AppliedEvents.Contains(Appear)) return false;
    if (AppliedEvents.Contains(Erase) && Entities.FindRef(Vareth) != EEidolonPresence::Oblivion) return false;
    if (AppliedEvents.Contains(Recover) && Entities.FindRef(Noah) != EEidolonPresence::Oblivion && Ending.IsNone()) return false;
    return Reality == TEXT("prime") || Reality == TEXT("oblivion") || Reality == TEXT("alternate_valdris");
}
bool FEidolonRealityState::Apply(FName Event, int32 Chapter, int32 ExpectedRevision)
{
    if (!IsValid() || !Ending.IsNone() || ExpectedRevision != Revision || Revision == MAX_int32 || AppliedEvents.Contains(Event)) return false;
    auto Next = *this;
    auto FamilyMemory = [&Next](FName Memory) { for (auto& Pair : Next.CurrentMemories) Pair.Value = Memory; };
    if (Event == Appear && Chapter == 2 && Next.Entities.FindRef(Noah) == EEidolonPresence::Absent)
    {
        Next.Entities[Noah] = EEidolonPresence::Present;
        FamilyMemory(TEXT("family_with_noah"));
    }
    else if (Event == Erase && Chapter == 10 && Next.AppliedEvents.Contains(Appear) && Next.Entities.FindRef(Vareth) == EEidolonPresence::Present)
    {
        Next.Entities[Vareth] = EEidolonPresence::Oblivion;
        Next.Entities[Survivor] = EEidolonPresence::Oblivion;
        Next.OriginalMemories.Add(TEXT("vareth_survivor"));
    }
    else if (Event == Enter && Chapter == 20 && Next.AppliedEvents.Contains(Erase))
    {
        Next.Flags.Add(TEXT("oblivion_unlocked"));
        Next.Reality = TEXT("oblivion");
    }
    else if (Event == TEXT("alternate_family") && Chapter == 21 && Next.Flags.Contains(TEXT("oblivion_unlocked")))
    {
        Next.Reality = TEXT("alternate_valdris");
        Next.OriginalMemories.Add(TEXT("family_alternate"));
    }
    else if (Event == Disappear && Chapter == 22 && Next.AppliedEvents.Contains(Appear) && Next.AppliedEvents.Contains(TEXT("alternate_family")) && Next.Flags.Contains(TEXT("oblivion_unlocked")))
    {
        Next.Entities[Noah] = EEidolonPresence::Oblivion;
        Next.Reality = TEXT("prime");
        Next.OriginalMemories.Add(TEXT("family_with_noah"));
        FamilyMemory(TEXT("family_before_noah"));
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
    for (auto& Pair : Next.CurrentMemories) Pair.Value = Choice == TEXT("origin") ? FName(TEXT("family_before_noah")) : FName(TEXT("family_with_noah"));
    if (Choice == TEXT("freedom")) Next.Flags.Add(TEXT("reality_stable_passages"));
    Next.AppliedEvents.Add(TEXT("ending_selected"));
    ++Next.Revision;
    if (!Next.IsValid()) return false;
    *this = MoveTemp(Next);
    return true;
}
