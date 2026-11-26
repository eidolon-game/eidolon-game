#include "EidolonLegacyMigration.h"
#include "../EidolonCampaignSave.h"
#include "../Discrepancy/EidolonRealitySave.h"
#include "Kismet/GameplayStatics.h"

UEidolonRealitySave* FEidolonLegacyMigration::Convert(UEidolonCampaignSave* Legacy)
{
    if (!IsInGameThread() || !Legacy || Legacy->Schema != 1 || Legacy->Chapter < 1 ||
        Legacy->Chapter > 26 || !Legacy->Ending.IsNone()) return nullptr;
    // Chapter 22+ requires evidence of scene transitions not stored by schema 1.
    // Chapter 20/21 are also ambiguous until explicit scene markers exist.
    if (Legacy->Chapter >= 20) return nullptr;
    auto State = FEidolonRealityState::Initial();
    if (Legacy->Chapter >= 3)
    {
        if (!Legacy->Flags.Contains(TEXT("noah_present")) ||
            !Legacy->Memories.Contains(TEXT("family_before_noah")) ||
            !State.Apply(TEXT("noah_appear"), 2, State.Revision)) return nullptr;
    }
    if (Legacy->Chapter >= 11)
    {
        if (Legacy->Flags.Contains(TEXT("vareth_present")) ||
            !Legacy->Memories.Contains(TEXT("vareth_survivor")) ||
            !State.Apply(TEXT("vareth_erase"), 10, State.Revision)) return nullptr;
    }
    if (Legacy->RealityRevision != State.Revision) return nullptr;
    auto* Candidate = NewObject<UEidolonRealitySave>();
    Candidate->Reality = MoveTemp(State);
    if (!UGameplayStatics::SaveGameToMemory(Legacy, Candidate->CampaignBytes) ||
        !Candidate->IsValid()) return nullptr;
    return Candidate;
}
