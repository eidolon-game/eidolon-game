#include "QuestProgressRules.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuestProgressTest, "Eidolon.Quest.OrderAndDedup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FQuestProgressTest::RunTest(const FString& Parameters)
{
    using namespace EidolonQuest;
    const TArray<FObjective> Steps = {{TEXT("first"),TEXT("register"),1},{TEXT("second"),TEXT("return"),1}};
    FProgress State;
    FEvent A{TEXT("a"),TEXT("register"),TEXT("kael"),TEXT("guild"),TEXT("mq_02"),2};
    FEvent B{TEXT("b"),TEXT("return"),TEXT("kael"),TEXT("guild"),TEXT("mq_02"),2};
    TestFalse(TEXT("Out of order"), Apply(Steps,TEXT("mq_02"),2,B,State));
    TestTrue(TEXT("Accept first"), Apply(Steps,TEXT("mq_02"),2,A,State));
    TestFalse(TEXT("Duplicate"), Apply(Steps,TEXT("mq_02"),2,A,State));
    TestTrue(TEXT("Accept second"), Apply(Steps,TEXT("mq_02"),2,B,State));
    TestTrue(TEXT("Complete"), IsComplete(Steps,State));
    return true;
}
#endif
