#pragma once
#include "CoreMinimal.h"
class UEidolonCampaignSave;
class UEidolonRealitySave;

// Creates a candidate only; never writes or modifies the legacy slot.
struct EIDOLON_API FEidolonLegacyMigration
{
    static UEidolonRealitySave* Convert(UEidolonCampaignSave* Legacy);
};
