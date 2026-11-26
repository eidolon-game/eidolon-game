#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EidolonTargetingPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonTargetScoreTest, "Eidolon.Core.Targeting.Score",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonTargetScoreTest::RunTest(const FString& Parameters)
{
    using Eidolon::Targeting::Score;
    TestTrue(TEXT("Centered beats peripheral"), Score(500,1,1000,.3) < Score(500,.5,1000,.3));
    TestTrue(TEXT("Near beats far"), Score(200,1,1000,.3) < Score(800,1,1000,.3));
    TestTrue(TEXT("Out of range"), Score(1001,1,1000,.3) < 0);
    TestTrue(TEXT("Behind camera"), Score(200,-1,1000,.3) < 0);
    TestTrue(TEXT("Invalid range"), Score(200,1,0,.3) < 0);
    TestTrue(TEXT("Zero distance"), Score(0,1,1000,.3) < 0);
    TestTrue(TEXT("Boundary accepted"), Score(1000,.3,1000,.3) >= 0);
    return true;
}
#endif
