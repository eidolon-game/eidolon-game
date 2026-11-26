#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonDoubleClearingGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonDoubleClearingGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
    bool BindCompletionActors(UWorld& World) override;
    bool TryCompleteControl(UEidolonCampaignSubsystem& Campaign, AEidolonCharacter& Player,
        const ATargetPoint& Control) override;
    void UpdateCompletionActors(bool bCompleted) override;
    FString GetControlPrompt() const override;
    void SpawnOptionalThreats() override;

private:
    TWeakObjectPtr<AStaticMeshActor> LatchHandle;
    FRotator RestPose = FRotator::ZeroRotator;
};
