#include "EidolonCampaignSubsystem.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonCampaignRules, "Eidolon.Campaign.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonCampaignRules::RunTest(const FString& Parameters)
{
    auto* Instance = NewObject<UGameInstance>();
    Instance->Init();
    auto* Campaign = Instance->GetSubsystem<UEidolonCampaignSubsystem>();
    if (!TestNotNull(TEXT("Campaign subsystem"), Campaign)) { Instance->Shutdown(); return false; }
    TestTrue(TEXT("Definitions loaded"), Campaign->NewCampaign());
    TestEqual(TEXT("First chapter"), Campaign->CurrentChapter().Id, 1);
    TestFalse(TEXT("Cannot skip unresolved chapter"), Campaign->AdvanceChapter());
    TestFalse(TEXT("Cannot select early ending"), Campaign->ChooseEnding(TEXT("freedom")));
    Campaign->SetFlag(TEXT("chapter_1_resolved"));
    TestTrue(TEXT("Advance resolved chapter"), Campaign->AdvanceChapter());
    TestFalse(TEXT("Wrong objective type"), Campaign->RecordObjective(TEXT("giant")));
    TestTrue(TEXT("First descent kills"), Campaign->RecordObjective(TEXT("dungeon"), 3));
    TestTrue(TEXT("Advance to discrepancy"), Campaign->AdvanceChapter());
    TestTrue(TEXT("Noah now present"), Campaign->HasFlag(TEXT("noah_present")));
    TestFalse(TEXT("Cannot skip floor"), Campaign->ReachFloor(false, 100));
    Instance->Shutdown();
    return true;
}
#endif
