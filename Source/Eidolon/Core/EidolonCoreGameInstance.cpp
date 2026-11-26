#include "EidolonCoreGameInstance.h"
#include "EidolonCoreLog.h"
void UEidolonCoreGameInstance::Init()
{
    Super::Init();
    UE_LOG(LogEidolonCore, Log, TEXT("Core game instance initialized"));
}
void UEidolonCoreGameInstance::Shutdown()
{
    UE_LOG(LogEidolonCore, Log, TEXT("Core game instance shutting down"));
    Super::Shutdown();
}
