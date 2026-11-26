#pragma once

#include "../EidolonGameMode.h"
#include "EidolonSluiceGameMode.generated.h"

class AStaticMeshActor;
class ATargetPoint;
class UEidolonCampaignSubsystem;

UCLASS()
class EIDOLON_API AEidolonSluiceGameMode : public AEidolonGameMode
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
    void UpdateSluices(uint8 Progress);

    TWeakObjectPtr<ATargetPoint> West;
    TWeakObjectPtr<ATargetPoint> East;
    TWeakObjectPtr<ATargetPoint> Exit;
    TWeakObjectPtr<AStaticMeshActor> Gate;
    TWeakObjectPtr<AStaticMeshActor> Water;
    FVector InitialWaterPosition = FVector::ZeroVector;
    FString Notice;
    bool bBound = false;
};
