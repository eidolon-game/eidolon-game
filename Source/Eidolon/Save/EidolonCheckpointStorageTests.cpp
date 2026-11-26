#include "EidolonLegacyMigration.h"
#include "../EidolonCampaignSave.h"
#include "../Discrepancy/EidolonRealitySave.h"
#include "Kismet/GameplayStatics.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonCheckpointStorageTest, "Eidolon.Save.CheckpointStorage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonCheckpointStorageTest::RunTest(const FString& Parameters)
{
    // Do not touch occupied slots. Run only in a disposable test profile.
    const int32 Slot=9, User=0;
    const FString A=TEXT("EidolonReality_9_0"), B=TEXT("EidolonReality_9_1");
    if (UGameplayStatics::DoesSaveGameExist(A,User) || UGameplayStatics::DoesSaveGameExist(B,User))
    { AddError(TEXT("Test slot occupied; refusing to modify")); return false; }
    struct FCleanup { FString A,B; int32 User; ~FCleanup(){ UGameplayStatics::DeleteGameInSlot(A,User); UGameplayStatics::DeleteGameInSlot(B,User); } } Cleanup{A,B,User};
    auto* S=FEidolonLegacyMigration::Convert(NewObject<UEidolonCampaignSave>());
    if (!TestNotNull(TEXT("Initial"),S)) return false;
    if (!TestTrue(TEXT("Generation 0"),UEidolonRealitySave::Store(S,Slot,User))) return false;
    if (!TestTrue(TEXT("Generation 1"),UEidolonRealitySave::Store(S,Slot,User))) return false;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Read copy B"),UGameplayStatics::LoadDataFromSlot(Bytes,B,User))) return false;
    Bytes.SetNum(8); // Header survives; payload is truncated.
    if (!TestTrue(TEXT("Simulate interrupted write"),UGameplayStatics::SaveDataToSlot(Bytes,B,User))) return false;
    auto* Recovered=UEidolonRealitySave::Load(Slot,User);
    if (!TestNotNull(TEXT("Recovered previous generation"),Recovered)) return false;
    TestEqual(TEXT("Generation 0 recovered"),Recovered->Generation,int64(0));
    TestFalse(TEXT("Stale writer rejected"),UEidolonRealitySave::Store(S,Slot,User));
    TestTrue(TEXT("Repair copy"),UEidolonRealitySave::Store(Recovered,Slot,User));
    if (!TestTrue(TEXT("Read repaired copy"),UGameplayStatics::LoadDataFromSlot(Bytes,B,User))) return false;
    Bytes[4]=2; // Recognized magic, future outer format.
    if (!TestTrue(TEXT("Inject future format"),UGameplayStatics::SaveDataToSlot(Bytes,B,User))) return false;
    TestNull(TEXT("Future format blocks downgrade"),UEidolonRealitySave::Load(Slot,User));
    TestFalse(TEXT("Future format cannot be overwritten"),UEidolonRealitySave::Store(Recovered,Slot,User));
    TArray<uint8> After;
    TestTrue(TEXT("Read protected bytes"),UGameplayStatics::LoadDataFromSlot(After,B,User));
    TestTrue(TEXT("Future copy unchanged"),Bytes==After);
    return true;
}
#endif
