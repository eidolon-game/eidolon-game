#include "EidolonCampaignTransaction.h"
#include "EidolonLegacyMigration.h"
#include "../EidolonCampaignSave.h"
#include "../Discrepancy/EidolonRealitySave.h"
#include "Kismet/GameplayStatics.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonTxTest, "Eidolon.Save.TransactionRollback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonTxTest::RunTest(const FString& Parameters)
{
    auto* Legacy=NewObject<UEidolonCampaignSave>();
    Legacy->Chapter=2;
    auto* S=FEidolonLegacyMigration::Convert(Legacy);
    if (!TestNotNull(TEXT("Candidate"),S)) return false;
    FEidolonCampaignTransaction T;
    T.EventId=FGuid::NewGuid(); T.Source=TEXT("kael"); T.Target=TEXT("noah");
    T.Type=TEXT("noah_reveal"); T.Quest=TEXT("mq_02"); T.Chapter=2;
    T.ExpectedRevision=0; T.Checkpoint=TEXT("home_reveal"); T.bQuestValidated=true;
    T.RewardId=TEXT("test_reward"); T.Gold=10;
    const auto Original=S->CampaignBytes;
    auto Fail=[](UEidolonRealitySave*,int32,int32){return false;};
    auto Pass=[](UEidolonRealitySave* Candidate,int32,int32){++Candidate->Generation;return true;};
    using R=EEidolonCommitResult;
    TestTrue(TEXT("Write failure"),FEidolonCampaignTransactionAdapter::Commit(S,T,0,0,Fail)==R::StorageFailed);
    TestTrue(TEXT("Rollback all bytes"),S->CampaignBytes==Original);
    TestEqual(TEXT("Revision unchanged"),S->Reality.Revision,0);
    TestTrue(TEXT("No dedup on failure"),S->ProcessedEvents.IsEmpty() && S->ClaimedRewards.IsEmpty());
    TestTrue(TEXT("Retry succeeds"),FEidolonCampaignTransactionAdapter::Commit(S,T,0,0,Pass)==R::Committed);
    TestTrue(TEXT("Replay suppressed"),FEidolonCampaignTransactionAdapter::Commit(S,T,0,0,Pass)==R::AlreadyApplied);
    auto* Restored=Cast<UEidolonCampaignSave>(UGameplayStatics::LoadGameFromMemory(S->CampaignBytes));
    if (!TestNotNull(TEXT("Restored campaign"),Restored)) return false;
    TestEqual(TEXT("Reward once"),Restored->Gold,10);
    TestTrue(TEXT("Noah present"),Restored->Flags.Contains(TEXT("noah_present")));
    T.EventId=FGuid::NewGuid(); T.ExpectedRevision=1; T.Type=TEXT("test_reward_event");
    TestTrue(TEXT("Same reward ID new GUID rejected"),FEidolonCampaignTransactionAdapter::Commit(S,T,0,0,Pass)==R::Rejected);
    return true;
}
#endif
