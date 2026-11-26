#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EidolonCampaignSave.generated.h"

UCLASS()
class EIDOLON_API UEidolonCampaignSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Schema = 1;
    UPROPERTY(SaveGame) int32 Chapter = 1;
    UPROPERTY(SaveGame) int32 ObjectiveCount = 0;
    UPROPERTY(SaveGame) int32 Experience = 0;
    UPROPERTY(SaveGame) int32 Gold = 0;
    UPROPERTY(SaveGame) int32 AbyssFloor = 0;
    UPROPERTY(SaveGame) int32 StairFloor = 0;
    UPROPERTY(SaveGame) int32 GiantsDefeated = 0;
    UPROPERTY(SaveGame) int32 TitansDefeated = 0;
    UPROPERTY(SaveGame) int32 RealityRevision = 0;
    // Missing in older checkpoints, which begin the ordered sequence at zero.
    UPROPERTY(SaveGame) int32 FirstDescentCursor = 0;
    // Authored progress has separate receipts from the experimental Arena counter.
    UPROPERTY(SaveGame) int32 AuthoredAbyssCompletedFloor = 0;
    UPROPERTY(SaveGame) bool bAuthoredAbyssRicordaHeard = false;
    UPROPERTY(SaveGame) uint8 AbyssSluiceProgress = 0;
    UPROPERTY(SaveGame) uint8 AbyssMarrowLatchStage = 0;
    UPROPERTY(SaveGame) uint8 AbyssCounterweightLocation = 0;
    UPROPERTY(SaveGame) uint32 AbyssCounterweightRevision = 0;
    UPROPERTY(SaveGame) uint8 AbyssFirstMirrorOrientation = 0;
    UPROPERTY(SaveGame) uint8 AbyssSecondMirrorOrientation = 0;
    UPROPERTY(SaveGame) uint32 AbyssMirrorRevision = 0;
    UPROPERTY(SaveGame) uint8 AbyssPincerWinchStage = 0;
    UPROPERTY(SaveGame) uint8 AbyssBranchLatchRoute = 0;
    UPROPERTY(SaveGame) TSet<FName> Flags;
    UPROPERTY(SaveGame) TSet<FName> Memories;
    UPROPERTY(SaveGame) TMap<FName, int32> Relationships;
    UPROPERTY(SaveGame) TMap<FName, int32> Inventory;
    UPROPERTY(SaveGame) FName Ending;
};
