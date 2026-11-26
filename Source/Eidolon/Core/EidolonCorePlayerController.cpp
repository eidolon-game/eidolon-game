#include "EidolonCorePlayerController.h"
#include "EidolonLockOnComponent.h"
#include "GameFramework/Pawn.h"
bool AEidolonCorePlayerController::ToggleLockOn()
{
    if (!IsLocalController() || !GetPawn()) return false;
    auto* Lock = GetPawn()->FindComponentByClass<UEidolonLockOnComponent>();
    if (!Lock) return false;
    if (Lock->GetTarget()) { Lock->Release(); return false; }
    return Lock->Acquire();
}
void AEidolonCorePlayerController::OnUnPossess()
{
    if (GetPawn())
        if (auto* Lock = GetPawn()->FindComponentByClass<UEidolonLockOnComponent>())
            Lock->Release();
    Super::OnUnPossess();
}
