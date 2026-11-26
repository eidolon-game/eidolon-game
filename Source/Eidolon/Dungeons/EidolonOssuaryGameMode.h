#pragma once

#include "CoreMinimal.h"
#include "../EidolonGameMode.h"
#include "EidolonOssuaryGameMode.generated.h"

class AEidolonCharacter;
class AEidolonOssuaryTick;
class AStaticMeshActor;
class ATargetPoint;
class UEidolonCampaignSubsystem;

UCLASS()
class EIDOLON_API AEidolonOssuaryGameMode : public AEidolonGameMode
{
    GENERATED_BODY()

public:
    virtual void StartPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual void Interact(AEidolonCharacter* Player) override;
    virtual void ReloadCheckpoint() override;
    virtual FString GetPrompt() const override;
    virtual FString GetNarrative() const override;
    virtual FString GetDialogue() const override { return FString(); }
    virtual bool ShowsArenaObjectiveCounter() const override { return false; }
    virtual void SelectEnding(int32) override {}
    virtual void RecoverMemory(AEidolonCharacter*) override {}

private:
    UEidolonCampaignSubsystem* GetCampaign() const;
    bool BindFloor();
    bool CanInteractAt(const AEidolonCharacter& Player, const ATargetPoint* Marker) const;
    void SpawnTick(UEidolonCampaignSubsystem& Campaign);
    void HandleDefeat(AEidolonOssuaryTick& Tick);
    void CommitEncounter(UEidolonCampaignSubsystem& Campaign);
    void SetGateOpen(bool bOpen);

    TWeakObjectPtr<ATargetPoint> Arena;
    TWeakObjectPtr<ATargetPoint> TickSpawn;
    TWeakObjectPtr<ATargetPoint> Exit;
    TWeakObjectPtr<AStaticMeshActor> Gate;
    UPROPERTY() TObjectPtr<AEidolonOssuaryTick> ActiveTick;
    int32 SpawnRevision = INDEX_NONE;
    bool bFloorBound = false;
    bool bDefeatAwaitingReceipt = false;
    FString Notice;
};
