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
    UPROPERTY(SaveGame) TSet<FName> Flags;
    UPROPERTY(SaveGame) TSet<FName> Memories;
    UPROPERTY(SaveGame) TMap<FName, int32> Relationships;
    UPROPERTY(SaveGame) TMap<FName, int32> Inventory;
    UPROPERTY(SaveGame) FName Ending;
};
