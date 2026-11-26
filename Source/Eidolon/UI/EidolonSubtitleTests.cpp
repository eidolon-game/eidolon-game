#include "EidolonSubtitleModel.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonSubtitleTest, "Eidolon.UI.Subtitles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonSubtitleTest::RunTest(const FString& Parameters)
{
    auto* Model = NewObject<UEidolonSubtitleModel>();
    FEidolonSubtitleCue Cue;
    Cue.Id = FName(TEXT("test_cue"));
    Cue.Line = FText::FromString(TEXT("Test"));
    Cue.Duration = 2.f;
    TestFalse(TEXT("Disabled"), Model->Present(Cue, false));
    TestTrue(TEXT("Valid cue"), Model->Present(Cue, true));
    Model->Advance(-1.f);
    Model->Advance(1.f);
    TestTrue(TEXT("Still visible"), Model->IsVisible());
    Model->Advance(1.f);
    TestFalse(TEXT("Expired"), Model->IsVisible());
    TestTrue(TEXT("Cleared ID"), Model->GetActive().Id.IsNone());
    Cue.Duration = 0.f;
    TestFalse(TEXT("Invalid duration"), Model->Present(Cue, true));
    return true;
}
#endif
