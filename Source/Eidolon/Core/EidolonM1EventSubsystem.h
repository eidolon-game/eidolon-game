#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EidolonRuntimeContract.h"
#include "EidolonM1EventSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEidolonSandboxObservation, const FEidolonRuntimeEvent&, Event);

// Nonpersistent observation channel. It cannot advance quests or grant rewards.
UCLASS()
class EIDOLON_API UEidolonM1EventSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    EEidolonReceipt Publish(const FEidolonRuntimeEvent& Event);
    UPROPERTY(BlueprintAssignable, Category="Eidolon|M1") FEidolonSandboxObservation OnObservation;
    virtual void Deinitialize() override;
private:
    FEidolonSandboxLedger Ledger;
    bool bClosing = false;
    bool bDispatching = false;
};
