#include "EidolonRuntimeContract.h"

bool FEidolonRuntimeEvent::IsValid() const
{
    const auto Stable = [](const FString& Id) { return FEidolonGameplayEvent::IsStableId(Id); };
    if (SchemaVersion != 1 || !EventId.IsValid() || !Stable(SourceId) ||
        !Stable(TargetId) || !Stable(SemanticId)) return false;
    switch (Kind)
    {
        case EEidolonEventKind::CombatImpact:
        case EEidolonEventKind::EnemyDefeated:
        case EEidolonEventKind::PlayerDefeated:
        case EEidolonEventKind::AIIntent:
        case EEidolonEventKind::QuestObjective:
        case EEidolonEventKind::RealityTransition:
        case EEidolonEventKind::RewardCommitted: break;
        default: return false;
    }
    if (Domain == EEidolonEventDomain::Sandbox)
        return Chapter == 0 && QuestId.IsEmpty() && ExpectedRevision == INDEX_NONE &&
            (Kind == EEidolonEventKind::CombatImpact || Kind == EEidolonEventKind::EnemyDefeated ||
             Kind == EEidolonEventKind::PlayerDefeated || Kind == EEidolonEventKind::AIIntent);
    if (Domain != EEidolonEventDomain::Campaign || Chapter < 1 || Chapter > 26 ||
        ExpectedRevision < 0 || (!QuestId.IsEmpty() && !Stable(QuestId))) return false;
    return Kind != EEidolonEventKind::QuestObjective || !QuestId.IsEmpty();
}

bool FEidolonRuntimeEvent::SamePayload(const FEidolonRuntimeEvent& O) const
{
    return SchemaVersion == O.SchemaVersion && EventId == O.EventId && Kind == O.Kind &&
        Domain == O.Domain && SourceId == O.SourceId && TargetId == O.TargetId &&
        SemanticId == O.SemanticId && Chapter == O.Chapter && QuestId == O.QuestId &&
        ExpectedRevision == O.ExpectedRevision;
}

bool FEidolonRuntimeEvent::FromLegacy(const FEidolonGameplayEvent& Legacy, EEidolonEventKind InKind,
    int32 Revision, FEidolonRuntimeEvent& Out)
{
    if (!Legacy.IsValid()) return false;
    FEidolonRuntimeEvent Candidate;
    Candidate.Domain = EEidolonEventDomain::Campaign;
    Candidate.Kind = InKind;
    Candidate.EventId = Legacy.EventId;
    Candidate.SourceId = Legacy.SourceEntityId;
    Candidate.TargetId = Legacy.TargetEntityId;
    Candidate.SemanticId = Legacy.EventType;
    Candidate.Chapter = Legacy.ChapterId;
    Candidate.QuestId = Legacy.QuestId;
    Candidate.ExpectedRevision = Revision;
    if (!Candidate.IsValid()) return false;
    Out = MoveTemp(Candidate);
    return true;
}

EEidolonReceipt FEidolonSandboxLedger::Accept(const FEidolonRuntimeEvent& Event)
{
    if (!Event.IsValid()) return EEidolonReceipt::Invalid;
    if (Event.Domain != EEidolonEventDomain::Sandbox) return EEidolonReceipt::WrongDomain;
    if (const auto* Existing = Seen.Find(Event.EventId))
        return Existing->SamePayload(Event) ? EEidolonReceipt::Duplicate : EEidolonReceipt::Conflict;
    if (Seen.Num() >= Limit) return EEidolonReceipt::Capacity;
    Seen.Add(Event.EventId, Event);
    return EEidolonReceipt::Accepted;
}
