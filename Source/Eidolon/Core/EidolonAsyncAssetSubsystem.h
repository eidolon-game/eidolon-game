#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "EidolonAsyncAssetSubsystem.generated.h"

DECLARE_DELEGATE_OneParam(FEidolonAssetResult, UObject*);

UCLASS()
class EIDOLON_API UEidolonAsyncAssetSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    // Game-thread only. Caller retains a strong reference after completion if needed.
    // Cancellation and shutdown suppress the callback. Invalid request returns invalid GUID.
    FGuid Request(const FSoftObjectPath& Path, FEidolonAssetResult Completion);
    bool Cancel(FGuid RequestId);
    virtual void Deinitialize() override;
private:
    TMap<FGuid, TSharedPtr<FStreamableHandle>> Pending;
    bool bShuttingDown = false;
};
