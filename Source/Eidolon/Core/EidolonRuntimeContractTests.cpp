#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EidolonRuntimeContract.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonRuntimeContractTest, "Eidolon.Core.Runtime.Contract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonRuntimeContractTest::RunTest(const FString& Parameters)
{
    FEidolonRuntimeEvent E;
    E.EventId = FGuid::NewGuid();
    E.Kind = EEidolonEventKind::EnemyDefeated;
    E.SourceId = TEXT("m1_player");
    E.TargetId = TEXT("m1_enemy_01");
    E.SemanticId = TEXT("enemy_defeated");
    FEidolonSandboxLedger Ledger(1);
    TestTrue(TEXT("Valid sandbox event"), E.IsValid());
    TestTrue(TEXT("First observation"), Ledger.Accept(E) == EEidolonReceipt::Accepted);
    TestTrue(TEXT("Retry idempotent"), Ledger.Accept(E) == EEidolonReceipt::Duplicate);
    E.TargetId = TEXT("m1_enemy_02");
    TestTrue(TEXT("Conflicting ID"), Ledger.Accept(E) == EEidolonReceipt::Conflict);
    E.EventId = FGuid::NewGuid();
    TestTrue(TEXT("Capacity fails closed"), Ledger.Accept(E) == EEidolonReceipt::Capacity);
    E.Domain = EEidolonEventDomain::Campaign;
    E.Chapter = 2;
    E.ExpectedRevision = 0;
    TestTrue(TEXT("Campaign cannot enter sandbox"), Ledger.Accept(E) == EEidolonReceipt::WrongDomain);
    E.Kind = EEidolonEventKind::QuestObjective;
    TestFalse(TEXT("Quest context required"), E.IsValid());
    E.QuestId = TEXT("mq_02");
    TestTrue(TEXT("Quest context accepted"), E.IsValid());
    E.SourceId = TEXT("Kael");
    TestFalse(TEXT("Uppercase rejected"), E.IsValid());
    Ledger.Reset();
    E.SourceId = TEXT("m1_player");
    E.Domain = EEidolonEventDomain::Sandbox;
    E.Kind = EEidolonEventKind::EnemyDefeated;
    E.Chapter = 0; E.QuestId.Reset(); E.ExpectedRevision = INDEX_NONE;
    TestTrue(TEXT("New world ledger"), Ledger.Accept(E) == EEidolonReceipt::Accepted);
    return true;
}
#endif
