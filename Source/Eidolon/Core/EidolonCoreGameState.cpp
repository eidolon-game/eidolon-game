#include "EidolonCoreGameState.h"
bool AEidolonCoreGameState::SetRegion(FName NewRegion)
{
    if (!HasAuthority() || NewRegion.IsNone()) return false;
    if (Region != NewRegion)
    {
        Region = NewRegion;
        OnRegionChanged.Broadcast(Region);
    }
    return true;
}
