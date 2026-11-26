#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EidolonGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AEidolonGameMode();
    virtual void StartPlay() override;
    virtual void Interact(class AEidolonCharacter* Player);
    virtual void SelectEnding(int32 Choice);
    virtual void ReloadCheckpoint();
    virtual FString GetPrompt() const;
    virtual FString GetNarrative() const;
    virtual FString GetDialogue() const;
    virtual bool ShowsArenaObjectiveCounter() const { return true; }
    virtual void RecoverMemory(class AEidolonCharacter* Player);
private:
    void SpawnEncounter();
    void RetryEncounterSpawns();
    void BuildChapterScenery();
    UPROPERTY() TArray<TObjectPtr<AActor>> Scenery;
    TArray<int32> PendingEnemySlots;
    bool IsEncounterClear() const;
    bool HasLiveEncounterEnemies() const;
    bool CommitCheckpoint();
    FString Notice;
    int32 DialogueIndex = 0;
};
