#include "EidolonM1EventSubsystem.h"
#include "Engine/World.h"
#include "EidolonCoreLog.h"

EEidolonReceipt UEidolonM1EventSubsystem::Publish(const FEidolonRuntimeEvent& Event)
{
    if (!IsInGameThread() || bClosing || bDispatching || !GetWorld() ||
        GetWorld()->GetNetMode() == NM_Client) return EEidolonReceipt::Invalid;
    const auto Receipt = Ledger.Accept(Event);
    if (Receipt == EEidolonReceipt::Accepted)
    {
        TGuardValue<bool> Guard(bDispatching, true);
        OnObservation.Broadcast(Event);
    }
    else if (Receipt == EEidolonReceipt::Conflict || Receipt == EEidolonReceipt::Capacity)
        UE_LOG(LogEidolonCore, Warning, TEXT("M1 observation rejected: %d"), static_cast<int32>(Receipt));
    return Receipt;
}
void UEidolonM1EventSubsystem::Deinitialize()
{
    bClosing = true;
    OnObservation.Clear();
    Ledger.Reset();
    Super::Deinitialize();
}
