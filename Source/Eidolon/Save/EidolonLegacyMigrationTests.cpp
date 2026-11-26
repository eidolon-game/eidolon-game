#include "EidolonLegacyMigration.h"
#include "../EidolonCampaignSave.h"
#include "../Discrepancy/EidolonRealitySave.h"
#include "Kismet/GameplayStatics.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonMigrationTest, "Eidolon.Save.LegacyMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonMigrationTest::RunTest(const FString& Parameters)
{
    auto* Legacy = NewObject<UEidolonCampaignSave>();
    Legacy->Gold = 42;
    Legacy->Inventory.Add(TEXT("test_item"), 3);
    auto* Candidate = FEidolonLegacyMigration::Convert(Legacy);
    if (!TestNotNull(TEXT("Initial migration"), Candidate)) return false;
    auto* Restored = Cast<UEidolonCampaignSave>(UGameplayStatics::LoadGameFromMemory(Candidate->CampaignBytes));
    if (!TestNotNull(TEXT("Restored campaign"), Restored)) return false;
    TestEqual(TEXT("Gold retained"), Restored->Gold, 42);
    TestEqual(TEXT("Inventory retained"), Restored->Inventory.FindRef(TEXT("test_item")), 3);
    Legacy->Chapter = 3;
    TestNull(TEXT("Missing evidence rejected"), FEidolonLegacyMigration::Convert(Legacy));
    Legacy->Flags.Add(TEXT("noah_present"));
    Legacy->Memories.Add(TEXT("family_before_noah"));
    Legacy->RealityRevision = 1;
    TestNotNull(TEXT("Supported chapter 3"), FEidolonLegacyMigration::Convert(Legacy));
    Legacy->Chapter = 22;
    TestNull(TEXT("Ambiguous chapter rejected"), FEidolonLegacyMigration::Convert(Legacy));
    TestEqual(TEXT("Source untouched"), Legacy->Chapter, 22);
    return true;
}
#endif
