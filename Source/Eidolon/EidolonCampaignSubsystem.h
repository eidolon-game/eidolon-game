#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EidolonCampaignSubsystem.generated.h"
class UEidolonCampaignSave;
namespace EidolonAbyss { struct Event; }
namespace EidolonSluice { enum class Control : uint8; }
namespace EidolonMarrowLatch { enum class Stage : uint8; enum class Signal : uint8; }
namespace EidolonCounterweight { struct Progress; struct Event; }
namespace EidolonMirror { struct Progress; struct Event; }
namespace EidolonPincerWinch { enum class Stage : uint8; enum class Signal : uint8; }
namespace EidolonBranchLatch { enum class Route : uint8; }
namespace EidolonFirstDescent { struct Event; }

USTRUCT(BlueprintType)
struct FEidolonChapter
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 Id = 0;
    UPROPERTY(BlueprintReadOnly) int32 Act = 0;
    UPROPERTY(BlueprintReadOnly) FString Title;
    UPROPERTY(BlueprintReadOnly) FString Region;
    UPROPERTY(BlueprintReadOnly) FString Objective;
    UPROPERTY(BlueprintReadOnly) FName Type;
    UPROPERTY(BlueprintReadOnly) int32 Target = 0;
    UPROPERTY(BlueprintReadOnly) TArray<FString> Dialogue;
};

UCLASS()
class EIDOLON_API UEidolonCampaignSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    UFUNCTION(BlueprintCallable) bool NewCampaign();
    UFUNCTION(BlueprintCallable) bool LoadCampaign();
    UFUNCTION(BlueprintCallable) bool SaveCampaign();
    UFUNCTION(BlueprintPure) FEidolonChapter CurrentChapter() const;
    UFUNCTION(BlueprintPure) int32 GetObjectiveCount() const;
    UFUNCTION(BlueprintPure) int32 GetFloor(bool bStair) const;
    UFUNCTION(BlueprintPure) FName GetEnding() const;
    UFUNCTION(BlueprintPure) FString GetEpilogue() const;
    UFUNCTION(BlueprintPure) int32 GetLevel() const;
    UFUNCTION(BlueprintPure) FName GetRank() const;
    UFUNCTION(BlueprintCallable) bool RecordObjective(FName Type, int32 Count = 1);
    UFUNCTION(BlueprintCallable) bool AdvanceChapter();
    UFUNCTION(BlueprintCallable) bool ReachFloor(bool bStair, int32 Floor);
    UFUNCTION(BlueprintCallable) void RecordMemory(FName Memory);
    UFUNCTION(BlueprintCallable) void SetFlag(FName Flag);
    UFUNCTION(BlueprintPure) bool HasFlag(FName Flag) const;
    UFUNCTION(BlueprintCallable) void ApplyDiscrepancy(FName Memory, FName CurrentFlag, bool bPresent);
    UFUNCTION(BlueprintCallable) bool ChangeRelationship(FName Character, int32 Delta);
    UFUNCTION(BlueprintCallable) bool GrantItem(FName Item, int32 Count);
    UFUNCTION(BlueprintCallable) bool ChooseEnding(FName Ending);
    UFUNCTION(BlueprintPure) bool CanChooseEnding(FName Ending) const;
    UFUNCTION(BlueprintPure, Category="Eidolon|World") bool CanEnterWorldRegion(FName RegionId) const;
    int32 GetFirstDescentCursor() const;
    int32 GetAuthoredAbyssCompletedFloor() const;
    int32 GetAuthoredAbyssRevision() const;
    // The native map adapter attests to the specific encounter or story outcome.
    bool CommitAuthoredAbyssEvent(const EidolonAbyss::Event& Event);
    uint8 GetAbyssDrainProgress(int32 Floor) const;
    bool CommitAbyssDrain(int32 Floor, EidolonSluice::Control Control, uint8 ExpectedProgress);
    EidolonMarrowLatch::Stage GetAbyssMarrowLatchStage() const;
    bool CommitAbyssMarrowLatch(EidolonMarrowLatch::Signal Signal, EidolonMarrowLatch::Stage ExpectedStage);
    EidolonCounterweight::Progress GetAbyssCounterweightProgress() const;
    bool CommitAbyssCounterweight(const EidolonCounterweight::Event& Event);
    EidolonMirror::Progress GetAbyssMirrorProgress() const;
    bool CommitAbyssMirror(const EidolonMirror::Event& Event);
    EidolonPincerWinch::Stage GetAbyssPincerWinchStage() const;
    bool CommitAbyssPincerWinch(EidolonPincerWinch::Signal Signal, EidolonPincerWinch::Stage ExpectedStage);
    EidolonBranchLatch::Route GetAbyssBranchLatchRoute() const;
    bool CommitAbyssBranchLatch(EidolonBranchLatch::Route VerifiedLatch, EidolonBranchLatch::Route ExpectedRoute);
    // C++ runtime adapters must verify the actual producer and target before submitting.
    bool CommitFirstDescentEvent(const EidolonFirstDescent::Event& Event);
    bool BeginValdrisFirstDescent();
    bool RequestFirstDescentSurfaceReturn();
    bool CompleteFirstDescentSurfaceReturn();
    bool CommitRivenConversation();
    bool FinishSecondDay();
    bool HasPendingFirstDescentSurfaceReturn() const { return bFirstDescentReturnRequested; }
private:
    UPROPERTY() TObjectPtr<UEidolonCampaignSave> State;
    UPROPERTY() TArray<FEidolonChapter> Chapters;
    TMap<FName, FString> Epilogues;
    bool bFirstDescentReturnRequested = false;
    bool LoadDefinitions();
    bool IsValidCheckpoint(const UEidolonCampaignSave* Checkpoint) const;
    bool WriteCheckpoint(UEidolonCampaignSave* Snapshot) const;
};
