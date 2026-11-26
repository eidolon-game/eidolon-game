#pragma once
#include "CoreMinimal.h"
#include "EidolonGameplayEvent.h"
#include "EidolonRuntimeContract.generated.h"

UENUM(BlueprintType)
enum class EEidolonEventKind : uint8
{
    CombatImpact, EnemyDefeated, PlayerDefeated, AIIntent, QuestObjective, RealityTransition, RewardCommitted
};

UENUM(BlueprintType)
enum class EEidolonEventDomain : uint8 { Sandbox, Campaign };

USTRUCT(BlueprintType)
struct FEidolonRuntimeEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 SchemaVersion = 1;
    UPROPERTY(BlueprintReadOnly) FGuid EventId;
    UPROPERTY(BlueprintReadOnly) EEidolonEventKind Kind = EEidolonEventKind::CombatImpact;
    UPROPERTY(BlueprintReadOnly) EEidolonEventDomain Domain = EEidolonEventDomain::Sandbox;
    UPROPERTY(BlueprintReadOnly) FString SourceId;
    UPROPERTY(BlueprintReadOnly) FString TargetId;
    UPROPERTY(BlueprintReadOnly) FString SemanticId;
    UPROPERTY(BlueprintReadOnly) int32 Chapter = 0;
    UPROPERTY(BlueprintReadOnly) FString QuestId;
    UPROPERTY(BlueprintReadOnly) int32 ExpectedRevision = INDEX_NONE;

    bool IsValid() const;
    bool SamePayload(const FEidolonRuntimeEvent& Other) const;
    // Legacy transport has campaign context only; conversion never promotes sandbox facts.
    static bool FromLegacy(const FEidolonGameplayEvent& Legacy, EEidolonEventKind Kind,
        int32 Revision, FEidolonRuntimeEvent& Out);
};

enum class EEidolonReceipt : uint8 { Accepted, Duplicate, Conflict, Invalid, WrongDomain, Capacity };

// Session-only observation ledger; never use as durable reward authority.
class EIDOLON_API FEidolonSandboxLedger
{
public:
    explicit FEidolonSandboxLedger(int32 Capacity = 4096) : Limit(FMath::Max(1, Capacity)) {}
    EEidolonReceipt Accept(const FEidolonRuntimeEvent& Event);
    void Reset() { Seen.Reset(); }
private:
    int32 Limit;
    TMap<FGuid, FEidolonRuntimeEvent> Seen;
};
