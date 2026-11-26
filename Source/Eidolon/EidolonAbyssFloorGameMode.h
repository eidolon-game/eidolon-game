#pragma once

#include "CoreMinimal.h"
#include "EidolonArenaEnemy.h"
#include "EidolonGameMode.h"
#include "Quest/EidolonFirstDescentInteraction.h"
#include "EidolonAbyssFloorGameMode.generated.h"

class AStaticMeshActor;
class ATargetPoint;
class AEidolonCharacter;
class UEidolonCampaignSubsystem;
class AEidolonFirstDescentCrawler;

UCLASS()
class EIDOLON_API AEidolonAbyssFloorGameMode : public AEidolonGameMode
{
    GENERATED_BODY()

public:
    virtual void StartPlay() override;
    virtual void Interact(AEidolonCharacter* Player) override;
    virtual void ReloadCheckpoint() override;
    virtual FString GetPrompt() const override;
    virtual FString GetNarrative() const override;
    virtual FString GetDialogue() const override;
    bool ShowsArenaObjectiveCounter() const override { return false; }
    virtual void SelectEnding(int32) override {}
    virtual void RecoverMemory(AEidolonCharacter*) override {}

private:
    friend class AEidolonFirstDescentCrawler;
    static constexpr int32 CrawlerCount = 2;

    bool BindFloor();
    UEidolonCampaignSubsystem* GetCampaign() const;
    bool IsActivePlayer(const AEidolonCharacter* Player) const;
    bool CommitStep(UEidolonCampaignSubsystem& Campaign, int32 Step);
    void ArriveAtFloor(UEidolonCampaignSubsystem& Campaign, const AEidolonCharacter& Player);
    void SpawnNextCrawler();
    void ConfirmCrawlerDefeat(AEidolonFirstDescentCrawler& Crawler);
    void UpdateGate(int32 Cursor);

    FEidolonFirstDescentInteraction Interactions;
    TWeakObjectPtr<ATargetPoint> Entry;
    TWeakObjectPtr<ATargetPoint> Checkpoint;
    TWeakObjectPtr<ATargetPoint> Anomaly;
    TWeakObjectPtr<ATargetPoint> SurfaceExit;
    TWeakObjectPtr<ATargetPoint> CrawlerSpawns[CrawlerCount];
    TWeakObjectPtr<AStaticMeshActor> Gate;
    UPROPERTY() TObjectPtr<AEidolonFirstDescentCrawler> ActiveCrawler;
    int32 DefeatedCrawlers = 0;
    bool bFloorBound = false;
    bool bAnomalyObserved = false;
    FString FloorNotice;
};

UCLASS()
class EIDOLON_API AEidolonFirstDescentCrawler : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    virtual float TakeDamage(float Amount, const FDamageEvent& Event,
                             AController* Instigator, AActor* Causer) override;
};
