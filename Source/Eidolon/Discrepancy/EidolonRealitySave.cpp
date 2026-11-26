#include "EidolonRealitySave.h"
#include "EidolonCampaignSave.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    FString Key(int32 Slot, int32 Copy) { return FString::Printf(TEXT("EidolonReality_%d_%d"), Slot, Copy); }
    UEidolonRealitySave* Read(int32 Slot, int32 Copy, int32 User)
    {
        auto* S = Cast<UEidolonRealitySave>(UGameplayStatics::LoadGameFromSlot(Key(Slot, Copy), User));
        return S && S->IsValid() && S->Generation % 2 == Copy ? S : nullptr;
    }
}
bool UEidolonRealitySave::IsValid() const
{
    if (Schema != 2 || Generation < 0 || !Reality.IsValid() || CampaignBytes.IsEmpty()) return false;
    auto* C = Cast<UEidolonCampaignSave>(UGameplayStatics::LoadGameFromMemory(CampaignBytes));
    return C && C->Schema == 1 && C->Chapter >= 1 && C->Chapter <= 26
        && C->ObjectiveCount >= 0 && C->Experience >= 0 && C->Gold >= 0
        && C->AbyssFloor >= 0 && C->AbyssFloor <= 100 && C->StairFloor >= 0 && C->StairFloor <= 100
        && C->Ending == Reality.Ending;
}
UEidolonRealitySave* UEidolonRealitySave::Load(int32 Slot, int32 User)
{
    if (!IsInGameThread() || Slot < 0 || Slot > 9 || User < 0) return nullptr;
    auto* A = Read(Slot, 0, User);
    auto* B = Read(Slot, 1, User);
    return !A ? B : (!B || A->Generation >= B->Generation ? A : B);
}
bool UEidolonRealitySave::Store(UEidolonRealitySave* Snapshot, int32 Slot, int32 User)
{
    if (!IsInGameThread() || !Snapshot || Slot < 0 || Slot > 9 || User < 0 || !Snapshot->IsValid()) return false;
    auto* Previous = Load(Slot, User);
    if (Previous && Previous->Generation == MAX_int64) return false;
    const int64 Old = Snapshot->Generation;
    Snapshot->Generation = Previous ? Previous->Generation + 1 : 0;
    const int32 Copy = int32(Snapshot->Generation % 2);
    const bool bWritten = UGameplayStatics::SaveGameToSlot(Snapshot, Key(Slot, Copy), User);
    auto* Verified = bWritten ? Read(Slot, Copy, User) : nullptr;
    if (!Verified || Verified->Generation != Snapshot->Generation)
    {
        Snapshot->Generation = Old;
        return false;
    }
    return true;
}
