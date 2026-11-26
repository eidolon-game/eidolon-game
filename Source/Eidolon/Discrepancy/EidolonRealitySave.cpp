#include "EidolonRealitySave.h"
#include "../EidolonCampaignSave.h"
#include "../Save/CheckpointPolicy.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"

namespace
{
    using namespace EidolonCheckpoint;
    constexpr uint32 Magic = 0x45494453;
    constexpr uint32 Format = 1;
    constexpr int32 HeaderSize = 16;
    constexpr int32 MaxPayload = 64 * 1024 * 1024;
    FString Key(int32 Slot, int32 CopyIndex) { return FString::Printf(TEXT("EidolonReality_%d_%d"), Slot, CopyIndex); }
    void Put(TArray<uint8>& B, uint32 V) { for (int32 I=0; I<4; ++I) B.Add(uint8(V >> (I*8))); }
    uint32 Get(const TArray<uint8>& B, int32 At) { uint32 V=0; for (int32 I=0; I<4; ++I) V |= uint32(B[At+I]) << (I*8); return V; }
    struct FRead { Copy Info{Kind::Missing}; UEidolonRealitySave* Save = nullptr; TArray<uint8> Bytes; };
    FRead Read(int32 Slot, int32 CopyIndex, int32 User)
    {
        FRead R;
        const FString Name = Key(Slot, CopyIndex);
        if (!UGameplayStatics::DoesSaveGameExist(Name, User)) return R;
        R.Info.kind = Kind::Unreadable;
        if (!UGameplayStatics::LoadDataFromSlot(R.Bytes, Name, User)) return R;
        R.Info.kind = Kind::Corrupt;
        if (R.Bytes.Num() < 4) return R;
        TArray<uint8> Payload;
        if (Get(R.Bytes,0) == Magic)
        {
            if (R.Bytes.Num() < 8) return R;
            if (Get(R.Bytes,4) > Format) { R.Info.kind = Kind::Future; return R; }
            if (Get(R.Bytes,4) != Format || R.Bytes.Num() < HeaderSize) return R;
            const uint32 Length = Get(R.Bytes,8);
            if (Length > MaxPayload || Length != uint32(R.Bytes.Num()-HeaderSize)) return R;
            Payload.Append(R.Bytes.GetData()+HeaderSize, int32(Length));
            if (FCrc::MemCrc32(Payload.GetData(), Payload.Num()) != Get(R.Bytes,12)) return R;
        }
        else
        {
            // Old draft schema-2 SaveGame files are deliberately not auto-overwritten.
            R.Info.kind = Kind::Unreadable;
            return R;
        }
        auto* S = Cast<UEidolonRealitySave>(UGameplayStatics::LoadGameFromMemory(Payload));
        if (!S) return R;
        if (S->Schema > 2) { R.Info.kind = Kind::Future; return R; }
        if (!S->IsValid() || S->Generation < 0 || S->Generation % 2 != CopyIndex) return R;
        R.Info = {Kind::Valid, S->Generation};
        R.Save = S;
        return R;
    }
}
bool UEidolonRealitySave::IsValid() const
{
    if (Schema != 2 || Generation < -1 || !Reality.IsValid() || CampaignBytes.IsEmpty() || CampaignBytes.Num() > MaxPayload) return false;
    for (const FGuid& Id : ProcessedEvents) if (!Id.IsValid()) return false;
    for (FName Id : ClaimedRewards) if (Id.IsNone()) return false;
    auto* C = Cast<UEidolonCampaignSave>(UGameplayStatics::LoadGameFromMemory(CampaignBytes));
    if (!C || C->Schema != 1 || C->Chapter < 1 || C->Chapter > 26 ||
        C->ObjectiveCount < 0 || C->Experience < 0 || C->Gold < 0 ||
        C->AbyssFloor < 0 || C->AbyssFloor > 100 || C->StairFloor < 0 || C->StairFloor > 100 ||
        C->GiantsDefeated < 0 || C->TitansDefeated < 0 || C->RealityRevision != Reality.Revision ||
        C->Ending != Reality.Ending) return false;
    for (const auto& P : C->Inventory) if (P.Key.IsNone() || P.Value < 0) return false;
    for (const auto& P : C->Relationships) if (P.Key.IsNone() || P.Value < -100 || P.Value > 100) return false;
    return true;
}
UEidolonRealitySave* UEidolonRealitySave::Load(int32 Slot, int32 User)
{
    if (!IsInGameThread() || Slot < 0 || Slot > 9 || User < 0) return nullptr;
    const auto A = Read(Slot,0,User), B = Read(Slot,1,User);
    const auto S = EidolonCheckpoint::Select(A.Info,B.Info);
    return S.blocked ? nullptr : S.newest == 0 ? A.Save : S.newest == 1 ? B.Save : nullptr;
}
bool UEidolonRealitySave::Store(UEidolonRealitySave* Snapshot, int32 Slot, int32 User)
{
    if (!IsInGameThread() || !Snapshot || Slot < 0 || Slot > 9 || User < 0 || !Snapshot->IsValid()) return false;
    const auto A = Read(Slot,0,User), B = Read(Slot,1,User);
    const auto Plan = EidolonCheckpoint::Next(A.Info,B.Info,Snapshot->Generation);
    if (Plan.target < 0) return false;
    auto* Candidate = DuplicateObject<UEidolonRealitySave>(Snapshot, GetTransientPackage());
    Candidate->Generation = Plan.generation;
    TArray<uint8> Payload, Bytes;
    if (!UGameplayStatics::SaveGameToMemory(Candidate, Payload) || Payload.Num() > MaxPayload) return false;
    Put(Bytes,Magic); Put(Bytes,Format); Put(Bytes,uint32(Payload.Num()));
    Put(Bytes,FCrc::MemCrc32(Payload.GetData(),Payload.Num()));
    Bytes.Append(Payload);
    UGameplayStatics::SaveDataToSlot(Bytes, Key(Slot,Plan.target), User);
    // Exact readback defines logical acceptance even if the platform returned false.
    const auto Verified = Read(Slot,Plan.target,User);
    if (Verified.Info.kind != EidolonCheckpoint::Kind::Valid || Verified.Info.generation != Plan.generation || Verified.Bytes != Bytes) return false;
    Snapshot->Generation = Plan.generation;
    return true;
}
