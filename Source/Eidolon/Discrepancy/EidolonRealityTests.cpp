#include "EidolonRealityState.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonRealityTest, "Eidolon.Discrepancy.CanonicalTransitions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonRealityTest::RunTest(const FString& Parameters)
{
    auto S = FEidolonRealityState::Initial();
    TestTrue(TEXT("Initial valid"), S.IsValid());
    TestFalse(TEXT("Early recovery"), S.Apply(TEXT("noah.recover"),22,0));
    TestTrue(TEXT("Appearance"), S.Apply(TEXT("noah.appear"),2,0));
    TestFalse(TEXT("Duplicate"), S.Apply(TEXT("noah.appear"),2,1));
    TestFalse(TEXT("Stale revision"), S.Apply(TEXT("vareth.erase"),10,0));
    TestEqual(TEXT("Rejected events do not mutate revision"), S.Revision, 1);
    TestTrue(TEXT("Erasure"), S.Apply(TEXT("vareth.erase"),10,1));
    TestTrue(TEXT("Oblivion"), S.Apply(TEXT("oblivion.enter"),20,2));
    TestTrue(TEXT("Alternative"), S.Apply(TEXT("alternate.family"),21,3));
    TestTrue(TEXT("Disappearance"), S.Apply(TEXT("noah.disappear"),22,4));
    TestTrue(TEXT("Recovery"), S.Apply(TEXT("noah.recover"),22,5));
    TestTrue(TEXT("Original memory preserved"), S.OriginalMemories.Contains(TEXT("family.before_noah")));
    TestTrue(TEXT("Later memory preserved"), S.OriginalMemories.Contains(TEXT("family.with_noah")));
    TestFalse(TEXT("Secret ending denied"), S.SelectEnding(TEXT("freedom"),26,true,true,false,true));
    TestFalse(TEXT("Boss gate"), S.SelectEnding(TEXT("origin"),26,false,true,true,true));
    for (FName Choice : {FName(TEXT("custodian")), FName(TEXT("origin")), FName(TEXT("guild")), FName(TEXT("freedom"))})
    {
        auto Copy = S;
        TestTrue(TEXT("Ending reachable"), Copy.SelectEnding(Choice,26,true,true,true,true));
        TestTrue(TEXT("Ending valid"), Copy.IsValid());
        TestFalse(TEXT("Ending immutable"), Copy.SelectEnding(TEXT("origin"),26,true,true,true,true));
    }
    return true;
}
#endif
