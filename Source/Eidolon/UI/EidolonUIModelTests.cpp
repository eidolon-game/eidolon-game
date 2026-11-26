#include "EidolonUIModel.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonUIModelTest, "Eidolon.UI.Model", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonUIModelTest::RunTest(const FString& Parameters)
{
    auto* Model = NewObject<UEidolonUIModel>();
    TestTrue(TEXT("Default title"), Model->GetScreen() == EEidolonScreen::Title);
    TestFalse(TEXT("Root back"), Model->Back());
    TestTrue(TEXT("Open settings"), Model->Push(EEidolonScreen::Settings));
    TestFalse(TEXT("Duplicate settings"), Model->Push(EEidolonScreen::Settings));
    TestTrue(TEXT("Back"), Model->Back());
    TestTrue(TEXT("Title restored"), Model->GetScreen() == EEidolonScreen::Title);
    TestEqual(TEXT("Upper clamp"), FEidolonMeter::FromFraction(2.f).Fraction, 1.f);
    TestEqual(TEXT("Lower clamp"), FEidolonMeter::FromFraction(-1.f).Fraction, 0.f);
    TestFalse(TEXT("NaN unavailable"), FEidolonMeter::FromFraction(std::numeric_limits<float>::quiet_NaN()).bAvailable);
    TestFalse(TEXT("Missing mana"), UEidolonUIModel::Capture(nullptr).Mana.bAvailable);
    FEidolonAccessibilityOptions Options;
    Options.TextScale = 5.f;
    Options.Normalize();
    TestEqual(TEXT("Text scale cap"), Options.TextScale, 2.f);
    return true;
}
#endif
