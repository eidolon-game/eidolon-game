#pragma once

#include "../EidolonGameMode.h"
#include "EidolonSluiceGameMode.generated.h"

class AStaticMeshActor;
class ATargetPoint;
class UEidolonCampaignSubsystem;

struct FEidolonDrainLayout
{
    int32 Floor;
    FName FirstControl;
    FName SecondControl;
    FName Gate;
    double PartialWaterZ;
    double EmptyWaterZ;
    bool bRemoveThreatWhenDrained;
    const TCHAR* TitleEnglish;
    const TCHAR* TitleItalian;
    const TCHAR* BothEnglish;
    const TCHAR* BothItalian;
    const TCHAR* RemainingEnglish;
    const TCHAR* RemainingItalian;
    const TCHAR* OpenEnglish;
    const TCHAR* OpenItalian;
    const TCHAR* DuplicateEnglish;
    const TCHAR* DuplicateItalian;
};

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

protected:
    virtual FEidolonDrainLayout GetDrainLayout() const;
    virtual AActor* SpawnThreat(UWorld& World);

private:
    UEidolonCampaignSubsystem* GetCampaign() const;
    bool CanUseFloor(const UEidolonCampaignSubsystem& Campaign) const;
    void UpdateSluices(uint8 Progress);

    TWeakObjectPtr<ATargetPoint> First;
    TWeakObjectPtr<ATargetPoint> Second;
    TWeakObjectPtr<ATargetPoint> Exit;
    TWeakObjectPtr<AStaticMeshActor> Gate;
    TWeakObjectPtr<AStaticMeshActor> Water;
    TWeakObjectPtr<AActor> Threat;
    FVector InitialWaterPosition = FVector::ZeroVector;
    FString Notice;
    bool bBound = false;
};
