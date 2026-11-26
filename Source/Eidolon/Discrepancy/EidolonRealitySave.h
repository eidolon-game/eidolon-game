#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EidolonRealityState.h"
#include "EidolonRealitySave.generated.h"

UCLASS()
class EIDOLON_API UEidolonRealitySave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Schema = 2;
    UPROPERTY(SaveGame) int64 Generation = 0;
    UPROPERTY(SaveGame) FEidolonRealityState Reality;
    UPROPERTY(SaveGame) TArray<uint8> CampaignBytes;
    UPROPERTY(SaveGame) FName Checkpoint;
    bool IsValid() const;
    // Two alternating generations; never overwrite the newest valid copy.
    static UEidolonRealitySave* Load(int32 Slot, int32 User);
    static bool Store(UEidolonRealitySave* Snapshot, int32 Slot, int32 User);
};
