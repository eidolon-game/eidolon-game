#include "../../EidolonCampaignSubsystem.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonQACampaignBoundaries, "Eidolon.QA.Campaign.Boundaries", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonQACampaignBoundaries::RunTest(const FString& Parameters)
{
    auto* Instance = NewObject<UGameInstance>();
    Instance->Init();
    auto* Campaign = Instance->GetSubsystem<UEidolonCampaignSubsystem>();
    if (!TestNotNull(TEXT("Campaign"), Campaign)) { Instance->Shutdown(); return false; }
    for (const FName Ending : {FName(TEXT("custodian")), FName(TEXT("origin")), FName(TEXT("guild")), FName(TEXT("freedom"))})
    {
        if (!TestTrue(TEXT("New campaign"), Campaign->NewCampaign())) break;
        TestFalse(TEXT("Early ending"), Campaign->ChooseEnding(Ending));
        TestFalse(TEXT("Negative item"), Campaign->GrantItem(TEXT("qa_item"), -1));
        TestTrue(TEXT("Maximum item"), Campaign->GrantItem(TEXT("qa_item"), MAX_int32));
        TestFalse(TEXT("Item overflow"), Campaign->GrantItem(TEXT("qa_item"), 1));
        for (int32 Id = 1; Id <= 26; ++Id)
        {
            const auto C = Campaign->CurrentChapter();
            if (!TestEqual(TEXT("Chapter"), C.Id, Id)) break;
            TestFalse(TEXT("Negative objective"), Campaign->RecordObjective(C.Type, -1));
            if (Id == 4 || Id == 23)
            {
                const bool Stair = Id == 23;
                TestFalse(TEXT("Skip floor"), Campaign->ReachFloor(Stair, 2));
                TestFalse(TEXT("Out of range floor"), Campaign->ReachFloor(Stair, 101));
                for (int32 Floor = 1; Floor <= 100; ++Floor)
                    TestTrue(TEXT("Sequential floor"), Campaign->ReachFloor(Stair, Floor));
                TestFalse(TEXT("Repeat floor"), Campaign->ReachFloor(Stair, 100));
            }
            if (C.Target > 0)
            {
                TestTrue(TEXT("Objective capped"), Campaign->RecordObjective(C.Type, MAX_int32));
                TestEqual(TEXT("Objective count"), Campaign->GetObjectiveCount(), C.Target);
                TestFalse(TEXT("Repeat objective"), Campaign->RecordObjective(C.Type, 1));
            }
            Campaign->SetFlag(FName(*FString::Printf(TEXT("chapter_%d_resolved"), Id)));
            if (Id == 22)
            {
                TestFalse(TEXT("Noah recovery required"), Campaign->AdvanceChapter());
                Campaign->SetFlag(TEXT("noah_found_in_oblivion"));
            }
            if (Id < 26 && !TestTrue(TEXT("Advance"), Campaign->AdvanceChapter())) break;
        }
        if (Ending == FName(TEXT("freedom")))
        {
            TestFalse(TEXT("Secret locked"), Campaign->CanChooseEnding(Ending));
            Campaign->SetFlag(TEXT("lost_memories"));
            Campaign->SetFlag(TEXT("eidolon_aid"));
            TestFalse(TEXT("Missing third flag"), Campaign->CanChooseEnding(Ending));
            Campaign->SetFlag(TEXT("origin_truth"));
        }
        TestTrue(TEXT("Select ending"), Campaign->ChooseEnding(Ending));
        TestEqual(TEXT("Selected ending"), Campaign->GetEnding(), Ending);
        TestFalse(TEXT("Ending immutable"), Campaign->ChooseEnding(Ending));
        TestFalse(TEXT("Cannot advance after ending"), Campaign->AdvanceChapter());
    }
    Instance->Shutdown();
    return true;
}
#endif
