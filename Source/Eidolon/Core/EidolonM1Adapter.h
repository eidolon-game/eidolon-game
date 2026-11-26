#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EidolonRuntimeContract.h"
#include "EidolonM1Adapter.generated.h"

USTRUCT(BlueprintType)
struct FEidolonM1Snapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) float HealthFraction = 0;
    UPROPERTY(BlueprintReadOnly) float StaminaFraction = 0;
    UPROPERTY(BlueprintReadOnly) bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly) bool bDefeated = false;
};

// Read-only adapter to legacy pawn, no second combat authority.
UCLASS(ClassGroup=(Eidolon), meta=(BlueprintSpawnableComponent))
class EIDOLON_API UEidolonM1Adapter : public UActorComponent
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Eidolon|M1") FEidolonM1Snapshot Capture() const;
    // Called once at confirmed death by the authoritative producer. Retry with identical ID.
    EEidolonReceipt ObserveDeath(const FGuid& EventId, const FString& SourceId,
        const FString& TargetId, bool bPlayerDefeated);
};
