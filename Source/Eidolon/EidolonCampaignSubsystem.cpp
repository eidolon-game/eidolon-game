#include "EidolonCampaignSubsystem.h"
#include "EidolonCampaignSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

namespace { const FString Slot = TEXT("EidolonCampaign01"); }

void UEidolonCampaignSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadDefinitions();
    if (!LoadCampaign()) NewCampaign();
}

bool UEidolonCampaignSubsystem::LoadDefinitions()
{
    FString Data;
    if (!FFileHelper::LoadFileToString(Data, *FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Eidolon/Data/campaign.json")))) return false;
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data), Root) || !Root.IsValid()) return false;
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Root->TryGetArrayField(TEXT("chapters"), Values) || Values->Num() != 26) return false;
    TArray<FEidolonChapter> Parsed;
    for (const auto& Value : *Values)
    {
        const auto Object = Value->AsObject();
        if (!Object.IsValid()) return false;
        FEidolonChapter C;
        C.Id = Object->GetIntegerField(TEXT("id"));
        C.Act = Object->GetIntegerField(TEXT("act"));
        C.Title = Object->GetStringField(TEXT("title"));
        C.Region = Object->GetStringField(TEXT("region"));
        C.Objective = Object->GetStringField(TEXT("objective"));
        C.Type = FName(*Object->GetStringField(TEXT("type")));
        C.Target = Object->GetIntegerField(TEXT("target"));
        if (C.Id != Parsed.Num()+1 || C.Act < 1 || C.Act > 10 || C.Target < 0) return false;
        Parsed.Add(C);
    }
    Chapters = MoveTemp(Parsed);
    return true;
}

bool UEidolonCampaignSubsystem::NewCampaign()
{
    State = NewObject<UEidolonCampaignSave>(this);
    State->Flags.Add(TEXT("family_original"));
    return Chapters.Num() == 26;
}

bool UEidolonCampaignSubsystem::LoadCampaign()
{
    auto* Loaded = Cast<UEidolonCampaignSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (!Loaded || Loaded->Schema != 1 || !Chapters.IsValidIndex(Loaded->Chapter-1) || Loaded->ObjectiveCount < 0) return false;
    State = Loaded;
    return true;
}

bool UEidolonCampaignSubsystem::SaveCampaign()
{
    return State && UGameplayStatics::SaveGameToSlot(State, Slot, 0);
}

FEidolonChapter UEidolonCampaignSubsystem::CurrentChapter() const
{
    return State && Chapters.IsValidIndex(State->Chapter-1) ? Chapters[State->Chapter-1] : FEidolonChapter();
}
int32 UEidolonCampaignSubsystem::GetObjectiveCount() const { return State ? State->ObjectiveCount : 0; }
int32 UEidolonCampaignSubsystem::GetLevel() const { return State ? 1 + State->Experience / 100 : 1; }
FName UEidolonCampaignSubsystem::GetRank() const
{
    if (!State) return TEXT("F");
    if (State->Chapter >= 24) return TEXT("Omega");
    if (State->Chapter >= 20) return TEXT("EX");
    if (State->Chapter >= 13) return TEXT("SSS");
    if (State->TitansDefeated > 0) return TEXT("SS");
    if (State->GiantsDefeated >= 100) return TEXT("S");
    if (State->AbyssFloor >= 100) return TEXT("A");
    if (State->AbyssFloor >= 80) return TEXT("B");
    if (State->AbyssFloor >= 60) return TEXT("C");
    if (State->AbyssFloor >= 40) return TEXT("D");
    if (State->AbyssFloor >= 20) return TEXT("E");
    return TEXT("F");
}

bool UEidolonCampaignSubsystem::RecordObjective(FName Type, int32 Count)
{
    const auto C = CurrentChapter();
    if (!State || !State->Ending.IsNone() || C.Id == 0 || C.Type != Type || Count <= 0 || C.Target <= 0 || State->ObjectiveCount >= C.Target) return false;
    const int32 Applied = FMath::Min(Count, C.Target - State->ObjectiveCount);
    State->ObjectiveCount += Applied;
    State->Experience = int32(FMath::Min<int64>(MAX_int32, int64(State->Experience) + int64(Applied)*25));
    if (Type == TEXT("giant")) State->GiantsDefeated += Applied;
    if (Type == TEXT("titan")) State->TitansDefeated += Applied;
    return true;
}

bool UEidolonCampaignSubsystem::ReachFloor(bool bStair, int32 Floor)
{
    if (!State || Floor < 1 || Floor > 100 || CurrentChapter().Id != (bStair ? 23 : 4)) return false;
    int32& Current = bStair ? State->StairFloor : State->AbyssFloor;
    if (Floor != Current+1) return false;
    Current = Floor;
    return true;
}

bool UEidolonCampaignSubsystem::AdvanceChapter()
{
    const auto C = CurrentChapter();
    if (!State || !State->Ending.IsNone() || C.Id <= 0 || C.Id >= 26 || State->ObjectiveCount < C.Target) return false;
    if (C.Id == 4 && State->AbyssFloor < 100) return false;
    if (C.Id == 23 && State->StairFloor < 100) return false;
    if (C.Target == 0 && !HasFlag(FName(*FString::Printf(TEXT("chapter_%d_resolved"), C.Id)))) return false;
    if (C.Id == 2) ApplyDiscrepancy(TEXT("family_before_noah"), TEXT("noah_present"), true);
    if (C.Id == 4) SetFlag(TEXT("sigil_aegis"));
    if (C.Id == 7) SetFlag(TEXT("sigil_horizon"));
    if (C.Id == 10) ApplyDiscrepancy(TEXT("vareth_survivor"), TEXT("vareth_present"), false);
    if (C.Id == 22) { ApplyDiscrepancy(TEXT("noah_forgotten"), TEXT("noah_present"), false); SetFlag(TEXT("noah_found_in_oblivion")); }
    State->Chapter++;
    State->ObjectiveCount = 0;
    return true;
}
void UEidolonCampaignSubsystem::RecordMemory(FName Memory) { if (State && !Memory.IsNone()) State->Memories.Add(Memory); }
void UEidolonCampaignSubsystem::SetFlag(FName Flag) { if (State && !Flag.IsNone()) State->Flags.Add(Flag); }
bool UEidolonCampaignSubsystem::HasFlag(FName Flag) const { return State && State->Flags.Contains(Flag); }
void UEidolonCampaignSubsystem::ApplyDiscrepancy(FName Memory, FName CurrentFlag, bool bPresent)
{
    if (!State || CurrentFlag.IsNone()) return;
    RecordMemory(Memory);
    if (bPresent) State->Flags.Add(CurrentFlag); else State->Flags.Remove(CurrentFlag);
    State->RealityRevision++;
}
bool UEidolonCampaignSubsystem::ChangeRelationship(FName Character, int32 Delta)
{
    if (!State || Character.IsNone()) return false;
    int32& Value = State->Relationships.FindOrAdd(Character);
    Value = int32(FMath::Clamp<int64>(int64(Value)+Delta,-100,100));
    return true;
}
bool UEidolonCampaignSubsystem::GrantItem(FName Item, int32 Count)
{
    if (!State || Item.IsNone() || Count <= 0) return false;
    int32& Value = State->Inventory.FindOrAdd(Item);
    if (Count > MAX_int32-Value) return false;
    Value += Count;
    return true;
}
bool UEidolonCampaignSubsystem::CanChooseEnding(FName Ending) const
{
    if (!State || !State->Ending.IsNone() || State->Chapter != 26 || State->ObjectiveCount < CurrentChapter().Target || CurrentChapter().Id != 26) return false;
    if (Ending == TEXT("freedom")) return HasFlag(TEXT("lost_memories")) && HasFlag(TEXT("eidolon_aid")) && HasFlag(TEXT("origin_truth"));
    return Ending == TEXT("custodian") || Ending == TEXT("origin") || Ending == TEXT("guild");
}
bool UEidolonCampaignSubsystem::ChooseEnding(FName Ending)
{
    if (!CanChooseEnding(Ending)) return false;
    State->Ending = Ending;
    return true;
}
