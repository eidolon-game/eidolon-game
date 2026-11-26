#pragma once

#include "../EidolonArenaEnemy.h"
#include "../EidolonGameMode.h"
#include "EidolonPincerWinchRules.h"
#include "TimerManager.h"
#include "EidolonCounterweightVaultGameMode.generated.h"

class AStaticMeshActor;
class ATargetPoint;
class UEidolonCampaignSubsystem;

UCLASS()
class EIDOLON_API AEidolonCounterweightVaultGameMode : public AEidolonGameMode
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
    bool BindVault();
    bool Commit(UEidolonCampaignSubsystem& Campaign, EidolonPincerWinch::Stage Stage,
                EidolonPincerWinch::Signal Signal);
    void UpdateVault(EidolonPincerWinch::Stage Stage);
    void SpawnPincer();
    void AnnounceCrawler();
    void SpawnCrawler();
    void OnEnemyDefeated(AEidolonArenaEnemy& Enemy);

    TWeakObjectPtr<ATargetPoint> Arena;
    TWeakObjectPtr<ATargetPoint> PincerSpawn;
    TWeakObjectPtr<ATargetPoint> CrawlerSpawn;
    TWeakObjectPtr<ATargetPoint> Winch;
    TWeakObjectPtr<ATargetPoint> Exit;
    TWeakObjectPtr<AStaticMeshActor> Gate;
    TWeakObjectPtr<AStaticMeshActor> FirstLatch;
    TWeakObjectPtr<AStaticMeshActor> SecondLatch;
    UPROPERTY() TObjectPtr<AEidolonArenaEnemy> ActiveEnemy;
    std::optional<EidolonPincerWinch::Signal> PendingDefeat;
    FTimerHandle CrawlerTimer;
    FString Notice;
    bool bBound = false;
};

UCLASS()
class EIDOLON_API AEidolonVaultCrawler : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    void BeginPlay() override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;
};
