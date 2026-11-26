#include "EidolonCampaignTransaction.h"
#include "../Discrepancy/EidolonRealitySave.h"
#include "../EidolonCampaignSave.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    bool Stable(const FString& S)
    {
        if (S.IsEmpty() || S.Len()>128 || S.StartsWith(TEXT("_")) || S.EndsWith(TEXT("_")) || S.Contains(TEXT("__"))) return false;
        for (TCHAR C : S) if (!((C>='a' && C<='z') || (C>='0' && C<='9') || C=='_')) return false;
        return true;
    }
    FName MapEvent(const FString& Type, int32 Chapter)
    {
        if (Chapter==2 && Type==TEXT("noah_reveal")) return TEXT("noah_appear");
        if (Chapter==10 && Type==TEXT("vareth_erasure_witnessed")) return TEXT("vareth_erase");
        if (Chapter==20 && Type==TEXT("oblivion_entered")) return TEXT("oblivion_enter");
        if (Chapter==21 && Type==TEXT("alternate_family_met")) return TEXT("alternate_family");
        if (Chapter==22 && Type==TEXT("noah_erasure_witnessed")) return TEXT("noah_disappear");
        if (Chapter==22 && Type==TEXT("noah_found_in_oblivion")) return TEXT("noah_recover");
        return NAME_None;
    }
}
EEidolonCommitResult FEidolonCampaignTransactionAdapter::Commit(UEidolonRealitySave* Current, const FEidolonCampaignTransaction& Tx,
    int32 Slot, int32 User, TFunctionRef<bool(UEidolonRealitySave*, int32, int32)> Persist)
{
    using R = EEidolonCommitResult;
    if (!IsInGameThread() || !Current || !Current->IsValid() || !Tx.EventId.IsValid() ||
        !Stable(Tx.Source) || (!Tx.Target.IsEmpty() && !Stable(Tx.Target)) ||
        !Stable(Tx.Type) || !Stable(Tx.Quest) || !Tx.bQuestValidated ||
        !Stable(Tx.Checkpoint.ToString()) || Tx.Gold<0 || Tx.ItemCount<0) return R::Rejected;
    if (Current->ProcessedEvents.Contains(Tx.EventId)) return R::AlreadyApplied;
    if (Tx.ExpectedRevision!=Current->Reality.Revision || !Current->Reality.Ending.IsNone()) return R::Rejected;
    auto* Candidate = DuplicateObject<UEidolonRealitySave>(Current, GetTransientPackage());
    auto* Campaign = Cast<UEidolonCampaignSave>(UGameplayStatics::LoadGameFromMemory(Candidate->CampaignBytes));
    if (!Campaign || Campaign->Chapter!=Tx.Chapter) return R::Rejected;
    const FName Event = MapEvent(Tx.Type,Tx.Chapter);
    if (!Event.IsNone())
    {
        if (Tx.Quest!=FString::Printf(TEXT("mq_%02d"), Tx.Chapter) ||
            !Candidate->Reality.Apply(Event,Tx.Chapter,Tx.ExpectedRevision)) return R::Rejected;
        Campaign->RealityRevision = Candidate->Reality.Revision;
        Campaign->Memories.Append(Candidate->Reality.OriginalMemories);
        if (Event==TEXT("noah_appear")) Campaign->Flags.Add(TEXT("noah_present"));
        if (Event==TEXT("vareth_erase")) Campaign->Flags.Remove(TEXT("vareth_present"));
        if (Event==TEXT("noah_disappear")) Campaign->Flags.Remove(TEXT("noah_present"));
        if (Event==TEXT("noah_recover")) Campaign->Flags.Add(TEXT("noah_found_in_oblivion"));
    }
    if (Tx.Gold || Tx.ItemCount)
    {
        if (Tx.RewardId.IsNone() || !Stable(Tx.RewardId.ToString()) ||
            Candidate->ClaimedRewards.Contains(Tx.RewardId) ||
            (Tx.ItemCount && !Stable(Tx.ItemId.ToString()))) return R::Rejected;
        const int32 Count = Campaign->Inventory.FindRef(Tx.ItemId);
        if (Tx.Gold>MAX_int32-Campaign->Gold || Tx.ItemCount>MAX_int32-Count) return R::Rejected;
        Campaign->Gold += Tx.Gold;
        if (Tx.ItemCount) Campaign->Inventory.Add(Tx.ItemId,Count+Tx.ItemCount);
        Candidate->ClaimedRewards.Add(Tx.RewardId);
    }
    Candidate->Checkpoint=Tx.Checkpoint;
    Candidate->ProcessedEvents.Add(Tx.EventId);
    if (!UGameplayStatics::SaveGameToMemory(Campaign,Candidate->CampaignBytes) || !Candidate->IsValid()) return R::Rejected;
    if (!Persist(Candidate,Slot,User)) return R::StorageFailed;
    Current->Reality=MoveTemp(Candidate->Reality);
    Current->CampaignBytes=MoveTemp(Candidate->CampaignBytes);
    Current->ProcessedEvents=MoveTemp(Candidate->ProcessedEvents);
    Current->ClaimedRewards=MoveTemp(Candidate->ClaimedRewards);
    Current->Checkpoint=Candidate->Checkpoint;
    Current->Generation=Candidate->Generation;
    return R::Committed;
}
