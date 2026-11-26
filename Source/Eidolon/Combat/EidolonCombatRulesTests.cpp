#include "Combat/EidolonCombatRules.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
using namespace EidolonCombat;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonCombatRulesTest,"Eidolon.Combat.Rules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEidolonCombatRulesTest::RunTest(const FString& Parameters)
{
    FState S; FAttack A;
    TestTrue(TEXT("Attack starts"),S.BeginAttack(A,1.f));
    TestEqual(TEXT("Cost"),S.Stamina,88.f);
    TestFalse(TEXT("No attack spam"),S.BeginAttack(A,1.1f));
    TestTrue(TEXT("Dodge"),S.Dodge(2.f));
    TestTrue(TEXT("I-frame"),S.Receive(A,2.1f,true)==EOutcome::Evaded);
    TestTrue(TEXT("Recovery vulnerable"),S.Receive(A,2.3f,true)==EOutcome::Hit);
    TestTrue(TEXT("Guard"),S.BeginBlock(3.f));
    TestTrue(TEXT("Perfect parry"),S.Receive(A,3.1f,true)==EOutcome::Parried);
    TestTrue(TEXT("Counter"),S.Counter(A,3.2f));
    TestFalse(TEXT("Counter consumed"),S.Counter(A,3.3f));
    S.Stamina=0;
    TestTrue(TEXT("Guard starts"),S.BeginBlock(5.f));
    TestTrue(TEXT("Break"),S.Receive(A,5.3f,true)==EOutcome::GuardBroken);
    FState Rear; Rear.BeginBlock(1.f);
    TestTrue(TEXT("Rear bypass"),Rear.Receive(A,1.1f,false)==EOutcome::Hit);
    FState Resistant; Resistant.Resistances[0]=.5f;
    Resistant.Receive(A,1.f,true);
    TestEqual(TEXT("Resistance"),Resistant.Health,90.f);
    return true;
}
#endif
