#include "EidolonCoreGameMode.h"
#include "EidolonCoreGameState.h"
#include "EidolonCorePlayerController.h"
AEidolonCoreGameMode::AEidolonCoreGameMode()
{
    GameStateClass = AEidolonCoreGameState::StaticClass();
    PlayerControllerClass = AEidolonCorePlayerController::StaticClass();
}
