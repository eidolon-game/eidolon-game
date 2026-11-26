#pragma once
#include "CoreMinimal.h"
#include "EidolonRealityState.generated.h"

UENUM(BlueprintType)
enum class EEidolonPresence : uint8 { Absent, Present, Oblivion };

USTRUCT(BlueprintType)
struct FEidolonRealityState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 Revision = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) TMap<FName, EEidolonPresence> Entities;
    UPROPERTY(SaveGame, BlueprintReadOnly) TSet<FName> OriginalMemories;
    UPROPERTY(SaveGame, BlueprintReadOnly) TMap<FName, FName> CurrentMemories;
    UPROPERTY(SaveGame, BlueprintReadOnly) TSet<FName> AppliedEvents;
    UPROPERTY(SaveGame, BlueprintReadOnly) TSet<FName> Flags;
    UPROPERTY(SaveGame, BlueprintReadOnly) FName Reality = TEXT("prime");
    UPROPERTY(SaveGame, BlueprintReadOnly) FName Ending;
    static FEidolonRealityState Initial();
    bool IsValid() const;
    bool Apply(FName Event, int32 Chapter, int32 ExpectedRevision);
    bool SelectEnding(FName Choice, int32 Chapter, bool bBossDefeated, bool bLostMemories, bool bAid, bool bTruth);
};
