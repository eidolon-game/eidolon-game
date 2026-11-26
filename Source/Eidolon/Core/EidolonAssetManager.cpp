#include "EidolonAssetManager.h"
#include "EidolonCoreLog.h"
void UEidolonAssetManager::StartInitialLoading()
{
    Super::StartInitialLoading();
    UE_LOG(LogEidolonCore, Log, TEXT("Asset manager initialized; region assets require configured scan paths"));
}
