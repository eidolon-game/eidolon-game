#include "EidolonPlayerSettings.h"
#include "Kismet/GameplayStatics.h"
#include "EidolonCoreLog.h"

namespace { const FString Slot = TEXT("eidolon_preferences_v1"); }

void UEidolonPlayerSettings::Sanitize()
{
    const auto Clamp = [](float Value, float Min, float Max)
    {
        return FMath::IsFinite(Value) ? FMath::Clamp(Value, Min, Max) : 1.f;
    };
    MouseSensitivity = Clamp(MouseSensitivity, .1f, 5.f);
    StickSensitivity = Clamp(StickSensitivity, .1f, 5.f);
    MasterVolume = Clamp(MasterVolume, 0.f, 1.f);
}
UEidolonPlayerSettings* UEidolonPlayerSettings::LoadPreferences()
{
    if (!IsInGameThread()) return nullptr;
    if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
        return Cast<UEidolonPlayerSettings>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
    auto* Settings = Cast<UEidolonPlayerSettings>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (!Settings || Settings->SchemaVersion != 1)
    {
        UE_LOG(LogEidolonCore, Warning, TEXT("Preferences unreadable or unsupported; refusing implicit overwrite"));
        return nullptr;
    }
    Settings->Sanitize();
    return Settings;
}
bool UEidolonPlayerSettings::SavePreferences()
{
    if (!IsInGameThread() || SchemaVersion != 1) return false;
    Sanitize();
    const bool bSaved = UGameplayStatics::SaveGameToSlot(this, Slot, 0);
    if (!bSaved) UE_LOG(LogEidolonCore, Error, TEXT("Preferences save failed"));
    return bSaved;
}
