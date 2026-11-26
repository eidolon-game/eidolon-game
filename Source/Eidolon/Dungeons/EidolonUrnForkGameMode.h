#pragma once

#include "../EidolonGameMode.h"
#include "EidolonBranchLatchRules.h"
#include "EidolonUrnForkGameMode.generated.h"

class AStaticMeshActor;
class ATargetPoint;
class UEidolonCampaignSubsystem;

UCLASS()
class EIDOLON_API AEidolonUrnForkGameMode : public AEidolonGameMode
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
    bool BindFork();
    void SpawnThreats();
    void UpdateGate(EidolonBranchLatch::Route Route);

    TWeakObjectPtr<ATargetPoint> UpperLatch;
    TWeakObjectPtr<ATargetPoint> LowerLatch;
    TWeakObjectPtr<ATargetPoint> Exit;
    TWeakObjectPtr<AStaticMeshActor> Gate;
    FString Notice;
    bool bBound = false;
};
