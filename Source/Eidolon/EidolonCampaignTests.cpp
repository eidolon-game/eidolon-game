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
    for (int32 ChapterId = 3; ChapterId <= 26; ++ChapterId)
    {
        const auto Chapter = Campaign->CurrentChapter();
        if (!TestEqual(TEXT("Sequential chapter"), Chapter.Id, ChapterId)) break;
        if (ChapterId == 4 || ChapterId == 23)
        {
            for (int32 Floor = 1; Floor <= 100; ++Floor)
                TestTrue(TEXT("Sequential floor"), Campaign->ReachFloor(ChapterId == 23, Floor));
        }
        if (Chapter.Target > 0) TestTrue(TEXT("Complete objective"), Campaign->RecordObjective(Chapter.Type, Chapter.Target));
        Campaign->SetFlag(FName(*FString::Printf(TEXT("chapter_%d_resolved"), ChapterId)));
        if (ChapterId == 22) Campaign->SetFlag(TEXT("noah_found_in_oblivion"));
        if (ChapterId < 26) TestTrue(TEXT("Advance chapter"), Campaign->AdvanceChapter());
    }
    TestFalse(TEXT("Secret ending locked"), Campaign->CanChooseEnding(TEXT("freedom")));
    Campaign->SetFlag(TEXT("lost_memories"));
    Campaign->SetFlag(TEXT("eidolon_aid"));
    Campaign->SetFlag(TEXT("origin_truth"));
    TestTrue(TEXT("Secret ending unlocked"), Campaign->CanChooseEnding(TEXT("freedom")));
    TestTrue(TEXT("Standard ending available"), Campaign->CanChooseEnding(TEXT("origin")));
    TestTrue(TEXT("Choose secret ending"), Campaign->ChooseEnding(TEXT("freedom")));
    TestFalse(TEXT("Ending cannot be replaced"), Campaign->ChooseEnding(TEXT("origin")));
    Instance->Shutdown();
    return true;
}
#endif
