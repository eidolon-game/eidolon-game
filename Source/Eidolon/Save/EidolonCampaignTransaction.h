#pragma once
#include "CoreMinimal.h"
class UEidolonRealitySave;

// Trusted game-thread caller must validate ordered quest progression and producer authority.
struct FEidolonCampaignTransaction
{
    FGuid EventId;
    FString Source, Target, Type, Quest;
    int32 Chapter = 0;
    int32 ExpectedRevision = -1;
    FName Checkpoint;
    FName RewardId;
    FName ItemId;
    int32 ItemCount = 0;
    int32 Gold = 0;
    bool bQuestValidated = false;
};
enum class EEidolonCommitResult { Rejected, AlreadyApplied, StorageFailed, Committed };
struct EIDOLON_API FEidolonCampaignTransactionAdapter
{
    static EEidolonCommitResult Commit(UEidolonRealitySave* Current, const FEidolonCampaignTransaction& Tx,
        int32 Slot, int32 User, TFunctionRef<bool(UEidolonRealitySave*, int32, int32)> Persist);
};
