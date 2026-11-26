#pragma once

#include "../EidolonGameMode.h"
#include "Templates/SubclassOf.h"
#include "EidolonAuthoredAbyssGameMode.generated.h"

class AEidolonArenaEnemy;
class AEidolonCharacter;
class AStaticMeshActor;
class ATargetPoint;
class UEidolonCampaignSubsystem;

struct FEidolonAbyssEncounter
{
    int32 Floor = 0;
    FName SpawnTag;
    FName GateTag;
    TSubclassOf<AEidolonArenaEnemy> EnemyClass;
    double SpawnHeight = 0;
    bool bRequiresLever = false;
    const TCHAR* TitleEnglish = TEXT("");
    const TCHAR* TitleItalian = TEXT("");
    const TCHAR* FightEnglish = TEXT("");
    const TCHAR* FightItalian = TEXT("");
};

UCLASS(Abstract)
class EIDOLON_API AEidolonAuthoredAbyssGameMode : public AEidolonGameMode
{
    GENERATED_BODY()

public:
    void StartPlay() override;
    AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    void Interact(AEidolonCharacter* Player) override;
    void ReloadCheckpoint() override;
    FString GetPrompt() const override;
    FString GetNarrative() const override;
    FString GetDialogue() const override { return {}; }
    bool ShowsArenaObjectiveCounter() const override { return false; }
    void SelectEnding(int32) override {}
    void RecoverMemory(AEidolonCharacter*) override {}

protected:
    virtual FEidolonAbyssEncounter GetEncounter() const { return {}; }

private:
    UEidolonCampaignSubsystem* GetCampaign() const;
    bool CanUseFloor(const UEidolonCampaignSubsystem& Campaign) const;
    bool BindFloor();
    bool CanInteractAt(const AEidolonCharacter& Player, const ATargetPoint* Marker) const;
    void SpawnEnemy(UEidolonCampaignSubsystem& Campaign);
    void HandleDefeat(AEidolonArenaEnemy& Enemy);
    void CommitEncounter(UEidolonCampaignSubsystem& Campaign);
    void SetGateOpen(bool bOpen);

    TWeakObjectPtr<ATargetPoint> Arena;
    TWeakObjectPtr<ATargetPoint> EnemySpawn;
    TWeakObjectPtr<ATargetPoint> Lever;
    TWeakObjectPtr<ATargetPoint> Exit;
    TWeakObjectPtr<AStaticMeshActor> Gate;
    UPROPERTY() TObjectPtr<AEidolonArenaEnemy> ActiveEnemy;
    int32 SpawnRevision = INDEX_NONE;
    bool bFloorBound = false;
    bool bDefeatAwaitingReceipt = false;
    FString Notice;
};
