#pragma once

#include "../EidolonGameMode.h"
#include "EidolonMirrorRules.h"
#include "EidolonPillarHallGameMode.generated.h"

class AStaticMeshActor;
class ATargetPoint;
class UEidolonCampaignSubsystem;

struct FEidolonMirrorBinding
{
    TWeakObjectPtr<ATargetPoint> Control;
    TWeakObjectPtr<AStaticMeshActor> Panel;
    TWeakObjectPtr<AStaticMeshActor> Pointer;
};

UCLASS()
class EIDOLON_API AEidolonPillarHallGameMode : public AEidolonGameMode
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
    bool BindMirrors();
    void UpdateMirrors(const EidolonMirror::Progress& Progress);
    void SpawnLeeches();

    FEidolonMirrorBinding Mirrors[2];
    TWeakObjectPtr<ATargetPoint> Exit;
    TWeakObjectPtr<AStaticMeshActor> Gate;
    FString Notice;
    bool bBound = false;
};
