#pragma once

#include "../EidolonGameMode.h"
#include "EidolonCounterweightRules.h"
#include "EidolonAmbulatoryGameMode.generated.h"

class AStaticMeshActor;
class ATargetPoint;
class UEidolonCampaignSubsystem;

UCLASS()
class EIDOLON_API AEidolonAmbulatoryGameMode : public AEidolonGameMode
{
    GENERATED_BODY()

public:
    AEidolonAmbulatoryGameMode();
    void StartPlay() override;
    void Tick(float DeltaSeconds) override;
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
    bool ApplyEvent(UEidolonCampaignSubsystem& Campaign, const EidolonCounterweight::Event& Event);
    void UpdateWeight(EidolonCounterweight::Progress Progress);
    void SpawnPatrol();

    TWeakObjectPtr<ATargetPoint> Sarcophagus;
    TWeakObjectPtr<ATargetPoint> Winch;
    TWeakObjectPtr<ATargetPoint> WeightSeat;
    TWeakObjectPtr<ATargetPoint> Exit;
    TWeakObjectPtr<AStaticMeshActor> Counterweight;
    TWeakObjectPtr<AStaticMeshActor> Gate;
    FTransform SourceTransform;
    FString Notice;
    bool bBound = false;
    bool bRecoveryAttempted = false;
};
