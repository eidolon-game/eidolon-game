#pragma once

#include "../EidolonGameMode.h"
#include "EidolonMarrowLatchRules.h"
#include "EidolonMarrowRamGameMode.generated.h"

class AEidolonArenaEnemy;
class AEidolonMarrowRam;
class AStaticMeshActor;
class ATargetPoint;
class UEidolonCampaignSubsystem;

UCLASS()
class EIDOLON_API AEidolonMarrowRamGameMode : public AEidolonGameMode
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

private:
    UEidolonCampaignSubsystem* GetCampaign() const;
    void SpawnRam();
    void OnBeamImpact(AEidolonMarrowRam& Ram, AActor& Obstacle);
    void OnRamDefeated(AEidolonArenaEnemy& Enemy);
    void RecordRelease(EidolonMarrowLatch::Signal Signal);
    bool SaveLatch(UEidolonCampaignSubsystem& Campaign, EidolonMarrowLatch::Signal Signal);
    void SetGateOpen(bool bOpen);

    TWeakObjectPtr<ATargetPoint> Arena;
    TWeakObjectPtr<ATargetPoint> RamSpawn;
    TWeakObjectPtr<ATargetPoint> Latch;
    TWeakObjectPtr<ATargetPoint> Exit;
    TWeakObjectPtr<AStaticMeshActor> Beam;
    TWeakObjectPtr<AStaticMeshActor> Gate;
    UPROPERTY() TObjectPtr<AEidolonMarrowRam> ActiveRam;
    std::optional<EidolonMarrowLatch::Signal> PendingRelease;
    FString Notice;
    bool bBound = false;
};
