#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EidolonQuestEventBridge.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonQuestBridgeTest, "Eidolon.Core.Runtime.QuestBridge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonQuestBridgeTest::RunTest(const FString& Parameters)
{
    FEidolonRuntimeEvent Event;
    Event.EventId = FGuid::NewGuid();
    Event.Domain = EEidolonEventDomain::Campaign;
    Event.Kind = EEidolonEventKind::QuestObjective;
    Event.SourceId = TEXT("kael");
    Event.TargetId = TEXT("guild_registrar");
    Event.SemanticId = TEXT("guild_register");
    Event.Chapter = 2;
    Event.QuestId = TEXT("mq_02");
    Event.ExpectedRevision = 0;
    FEidolonQuestEventFields A, B;
    TestTrue(TEXT("Campaign objective maps"), EidolonProjectQuestEvent(Event, A));
    TestTrue(TEXT("Retry maps"), EidolonProjectQuestEvent(Event, B));
    TestEqual(TEXT("Retry identity"), A.Id, B.Id);
    TestEqual(TEXT("Semantic type"), A.Type, FName(TEXT("guild_register")));
    Event.Kind = EEidolonEventKind::EnemyDefeated;
    TestFalse(TEXT("Raw combat death is not quest authorization"), EidolonProjectQuestEvent(Event, B));
    TestEqual(TEXT("Rejected projection leaves output unchanged"), B.Id, A.Id);
    return true;
}
#endif
