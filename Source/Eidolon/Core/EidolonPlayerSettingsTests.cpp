#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EidolonPlayerSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonSettingsTest, "Eidolon.Core.Settings.Sanitize",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonSettingsTest::RunTest(const FString& Parameters)
{
    auto* Settings = NewObject<UEidolonPlayerSettings>();
    Settings->MouseSensitivity = -2.f;
    Settings->StickSensitivity = 100.f;
    Settings->MasterVolume = 2.f;
    Settings->Sanitize();
    TestEqual(TEXT("Minimum sensitivity"), Settings->MouseSensitivity, .1f);
    TestEqual(TEXT("Maximum sensitivity"), Settings->StickSensitivity, 5.f);
    TestEqual(TEXT("Maximum volume"), Settings->MasterVolume, 1.f);
    Settings->MasterVolume = -1.f;
    Settings->Sanitize();
    TestEqual(TEXT("Minimum volume"), Settings->MasterVolume, 0.f);
    return true;
}
#endif
