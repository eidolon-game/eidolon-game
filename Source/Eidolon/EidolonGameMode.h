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
    void Interact(class AEidolonCharacter* Player);
    void SelectEnding(int32 Choice);
    void ReloadCheckpoint();
    FString GetPrompt() const;
    FString GetNarrative() const;
    FString GetDialogue() const;
    void RecoverMemory(class AEidolonCharacter* Player);
private:
    void SpawnEncounter();
    bool IsEncounterClear() const;
    bool CommitCheckpoint();
    FString Notice;
    int32 DialogueIndex = 0;
};
