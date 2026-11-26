#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonCounterweightVaultGameMode.h"
#include "EidolonRootclawGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonRootclawGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
    bool ConfigureEnemy(AEidolonArenaEnemy& Enemy) override;
    void SpawnOptionalThreats() override;
};

UCLASS()
class EIDOLON_API AEidolonRootclawCourtyardCrawler : public AEidolonVaultCrawler
{
    GENERATED_BODY()

public:
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;

private:
    FVector SpawnPosition = FVector::ZeroVector;
};
