#include "Combat/EidolonEnemyEncounter.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
using namespace EidolonCombat;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonCombatCycle2Test,"Eidolon.Combat.Cycle2",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEidolonCombatCycle2Test::RunTest(const FString&)
{
 TSet<int32> Hits; TestTrue(TEXT("First target"),ClaimHit(Hits,1));
 TestFalse(TEXT("Duplicate target"),ClaimHit(Hits,1)); TestTrue(TEXT("Second target"),ClaimHit(Hits,2));
 Hits.Reset(); TestTrue(TEXT("Next attack"),ClaimHit(Hits,1));
 FState S; FAttack A;
 S.Stamina=0; TestFalse(TEXT("No stamina"),S.BeginAttack(A,1));
 TestTrue(TEXT("Guard"),S.BeginBlock(1));
 TestTrue(TEXT("Parry"),S.Receive(A,1.1f,true)==EOutcome::Parried);
 TestFalse(TEXT("Counter rejected"),S.Counter(A,1.2f));
 TestTrue(TEXT("Guard preserved"),S.bBlocking);
 S.EndBlock(); S.Stamina=100;
 TestTrue(TEXT("Dodge"),S.Dodge(2));
 TestTrue(TEXT("Invulnerable"),S.Receive(A,2.1f,true)==EOutcome::Evaded);
 A.Damage=200; TestTrue(TEXT("Death hit"),S.Receive(A,2.3f,true)==EOutcome::Hit);
 TestFalse(TEXT("Dead"),S.IsAlive());
 TestTrue(TEXT("No duplicate death damage"),S.Receive(A,3,true)==EOutcome::Ignored);
 FEnemyEncounter E; E.bCustodian=true;
 TestFalse(TEXT("Bound shield"),E.CanTakeHealthDamage());
 TestTrue(TEXT("Anchor"),E.BreakAnchor(0)); TestFalse(TEXT("Duplicate anchor"),E.BreakAnchor(0));
 E.BreakAnchor(1); E.BreakAnchor(2); TestTrue(TEXT("Exposed"),E.CanTakeHealthDamage());
 E.HealthFraction=.35f; TestTrue(TEXT("Enraged"),E.Phase()==EEnemyPhase::CustodianEnraged);
 TestFalse(TEXT("Unblockable"),EnemyAttack(E.Phase()).Attack.bBlockable);
 TestTrue(TEXT("Heavy can reach boss"),PlayerAttack(true).Reach>=EnemyAttack(E.Phase()).Reach);
 return true;
}
#endif
