#include "EidolonAsyncAssetSubsystem.h"
#include "Engine/AssetManager.h"
#include "EidolonCoreLog.h"

FGuid UEidolonAsyncAssetSubsystem::Request(const FSoftObjectPath& Path, FEidolonAssetResult Completion)
{
    if (!IsInGameThread() || bShuttingDown || Path.IsNull() || !Completion.IsBound()) return FGuid();
    const FGuid Id = FGuid::NewGuid();
    auto Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        Path, FStreamableDelegate::CreateWeakLambda(this, [this, Id, Path, Completion]()
        {
            if (bShuttingDown || !Pending.Contains(Id)) return;
            // Keep the handle alive across the callback, including reentrant cancellation.
            const auto KeepAlive = Pending.FindRef(Id);
            Pending.Remove(Id);
            UObject* Asset = Path.ResolveObject();
            if (!Asset) UE_LOG(LogEidolonCore, Warning, TEXT("Async asset load failed: %s"), *Path.ToString());
            Completion.ExecuteIfBound(Asset);
        }), FStreamableManager::DefaultAsyncLoadPriority, false, true);
    if (!Handle.IsValid()) return FGuid();
    Pending.Add(Id, Handle);
    Handle->StartStalledHandle();
    return Id;
}
bool UEidolonAsyncAssetSubsystem::Cancel(FGuid RequestId)
{
    if (!IsInGameThread()) return false;
    TSharedPtr<FStreamableHandle> Handle;
    if (!Pending.RemoveAndCopyValue(RequestId, Handle)) return false;
    if (Handle) Handle->CancelHandle();
    return true;
}
void UEidolonAsyncAssetSubsystem::Deinitialize()
{
    bShuttingDown = true;
    for (auto& Entry : Pending)
        if (Entry.Value) Entry.Value->CancelHandle();
    Pending.Empty();
    Super::Deinitialize();
}
