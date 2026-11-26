#pragma once
#include "CoreMinimal.h"
// Template keeps department 02 independently compilable before Core PR lands.
// Instantiate with FEidolonGameplayEvent after Core contract review.
namespace EidolonCombat
{
template<class Envelope>
bool BuildEvent(Envelope& Out,const FGuid& EventId,const FString& Source,const FString& Target,const FString& Type,int32 Chapter,const FString& Quest)
{
    Envelope Candidate;
    Candidate.EventId=EventId; Candidate.SourceEntityId=Source; Candidate.TargetEntityId=Target;
    Candidate.EventType=Type; Candidate.ChapterId=Chapter; Candidate.QuestId=Quest;
    if(!Candidate.IsValid()) return false;
    Out=Candidate; return true;
}
}
