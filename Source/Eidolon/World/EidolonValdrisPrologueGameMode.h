#pragma once

#include "../EidolonGameMode.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "EidolonValdrisPrologueGameMode.generated.h"

class UEidolonCampaignSubsystem;
class AEidolonCharacter;

UCLASS()
class EIDOLON_API AEidolonValdrisPrologueGameMode : public AEidolonGameMode
{
    GENERATED_BODY()

public:
    void StartPlay() override;
    void Interact(AEidolonCharacter* Player) override;
    void ReloadCheckpoint() override;
    FString GetPrompt() const override;
    FString GetDialogue() const override;
    bool ShowsArenaObjectiveCounter() const override { return false; }
    void SelectEnding(int32) override {}
    void RecoverMemory(AEidolonCharacter*) override {}

private:
    UEidolonCampaignSubsystem* GetCampaign() const;

    FEidolonFirstDescentInteraction FarewellInteraction;
    FEidolonFirstDescentInteraction QuestInteraction;
    TWeakObjectPtr<AActor> Farewell;
    TMap<int32, TWeakObjectPtr<AActor>> Anchors;
    FString PromptOverride;
    FString Dialogue;
    int32 FarewellLine = 0;
};
