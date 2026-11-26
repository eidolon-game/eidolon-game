#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EidolonGameplayEvent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonGameplayEventTest, "Eidolon.Core.Events.Validation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonGameplayEventTest::RunTest(const FString& Parameters)
{
    FEidolonGameplayEvent Event;
    TestFalse(TEXT("Empty event rejected"), Event.IsValid());
    Event.EventId = FGuid::NewGuid();
    Event.SourceEntityId = TEXT("kael_veyr");
    Event.TargetEntityId = TEXT("abyss_guardian");
    Event.EventType = TEXT("enemy_defeated");
    Event.ChapterId = 4;
    TestTrue(TEXT("Valid event"), Event.IsValid());
    Event.ChapterId = 27;
    TestFalse(TEXT("Invalid chapter"), Event.IsValid());
    Event.ChapterId = 4;
    Event.TargetEntityId = TEXT("Abyss Guardian");
    TestFalse(TEXT("Display name rejected"), Event.IsValid());
    TestFalse(TEXT("Repeated separator"), FEidolonGameplayEvent::IsStableId(TEXT("a__b")));
    TestFalse(TEXT("Trailing separator"), FEidolonGameplayEvent::IsStableId(TEXT("a_")));
    TestTrue(TEXT("Stable floor identifier"), FEidolonGameplayEvent::IsStableId(TEXT("abyss_100")));
    return true;
}
#endif
