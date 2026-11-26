#include "EidolonCampaignSubsystem.h"
#include "EidolonCampaignSave.h"
#include "Dungeons/EidolonAbyssProgression.h"
#include "Dungeons/EidolonBranchLatchRules.h"
#include "Dungeons/EidolonCounterweightRules.h"
#include "Dungeons/EidolonSluiceRules.h"
#include "Dungeons/EidolonMarrowLatchRules.h"
#include "Dungeons/EidolonMirrorRules.h"
#include "Dungeons/EidolonPincerWinchRules.h"
#include "Quest/EidolonFirstDescentRules.h"
#include "World/EidolonWorldAccess.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"

namespace
{
    const FString& CampaignSlot()
    {
        static const FString ArenaSlot = TEXT("EidolonCampaign01");
        static const FString AuthoredSlot = TEXT("EidolonAuthoredCampaign01");
        return FParse::Param(FCommandLine::Get(), TEXT("EidolonAuthoredCampaign")) ? AuthoredSlot : ArenaSlot;
    }

    uint8 UEidolonCampaignSave::* DrainMaskMember(int32 Floor)
    {
        switch (Floor)
        {
            case EidolonAbyss::SluiceFloor: return &UEidolonCampaignSave::AbyssSluiceProgress;
            case EidolonAbyss::FernBasinFloor: return &UEidolonCampaignSave::AbyssFernBasinProgress;
            default: return nullptr;
        }
    }
    constexpr int32 FinalFirstDescentCursor = static_cast<int32>(EidolonFirstDescent::Steps.size());

    EidolonAbyss::Progress ReadAuthoredAbyssProgress(const UEidolonCampaignSave& Snapshot)
    {
        // The authored tutorial already proves that the first encounter was completed.
        const int32 TutorialFloor = Snapshot.Chapter >= EidolonAbyss::Chapter &&
            Snapshot.FirstDescentCursor == FinalFirstDescentCursor ? 1 : 0;
        return {FMath::Max(Snapshot.AuthoredAbyssCompletedFloor, TutorialFloor),
            Snapshot.bAuthoredAbyssRicordaHeard};
    }

    bool IsAuthoredAbyssMap(UWorld* World, int32 Floor)
    {
        if (!World || FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
            return false;

        // GetCurrentLevelName strips the world-specific PIE prefix.
        return UGameplayStatics::GetCurrentLevelName(World) == FString::Printf(TEXT("AbyssFloor%03d"), Floor);
    }

    void RevealNoah(UEidolonCampaignSave& Snapshot)
    {
        Snapshot.Memories.Add(TEXT("family_before_noah"));
        Snapshot.Flags.Add(TEXT("noah_present"));
        ++Snapshot.RealityRevision;
    }

    FString ReadText(const FJsonObject& Object, const TCHAR* Preferred, const TCHAR* Fallback)
    {
        FString Value;
        if (Object.TryGetStringField(Preferred, Value) && !Value.IsEmpty()) return Value;
        return Object.GetStringField(Fallback);
    }

    bool ResolveRegion(FName Id, EidolonWorld::Region& Destination)
    {
        using EidolonWorld::Region;
        if (Id == TEXT("valdris")) { Destination = Region::Valdris; return true; }
        if (Id == TEXT("giant_territory")) { Destination = Region::GiantTerritory; return true; }
        if (Id == TEXT("continent")) { Destination = Region::Continent; return true; }
        if (Id == TEXT("vareth")) { Destination = Region::Vareth; return true; }
        if (Id == TEXT("oblivion")) { Destination = Region::Oblivion; return true; }
        if (Id == TEXT("stair")) { Destination = Region::Stair; return true; }
        if (Id == TEXT("empyrea")) { Destination = Region::Empyrea; return true; }
        return false;
    }
}

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
    const bool bItalian = FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it");
    TArray<FEidolonChapter> Parsed;
    for (const auto& Value : *Values)
    {
        const auto Object = Value->AsObject();
        if (!Object.IsValid()) return false;
        FEidolonChapter C;
        C.Id = Object->GetIntegerField(TEXT("id"));
        C.Act = Object->GetIntegerField(TEXT("act"));
        C.Title = ReadText(*Object, bItalian ? TEXT("title") : TEXT("title_en"), TEXT("title"));
        C.Region = ReadText(*Object, bItalian ? TEXT("region") : TEXT("region_en"), TEXT("region"));
        C.Objective = ReadText(*Object, bItalian ? TEXT("objective_it") : TEXT("objective"), TEXT("objective"));
        C.Type = FName(*Object->GetStringField(TEXT("type")));
        C.Target = Object->GetIntegerField(TEXT("target"));
        if (C.Id != Parsed.Num()+1 || C.Act < 1 || C.Act > 10 || C.Target < 0) return false;
        const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
        if (Object->TryGetArrayField(TEXT("dialogue"), Lines))
            for (const auto& Line : *Lines)
            {
                const auto Entry = Line->AsObject();
                if (Entry.IsValid())
                {
                    const FString Speaker = ReadText(*Entry, bItalian ? TEXT("speaker") : TEXT("speaker_en"), TEXT("speaker"));
                    const FString LineText = ReadText(*Entry, bItalian ? TEXT("text") : TEXT("text_en"), TEXT("text"));
                    C.Dialogue.Add(Speaker + TEXT(": ") + LineText);
                }
            }
        Parsed.Add(C);
    }
    const TArray<TSharedPtr<FJsonValue>>* Endings = nullptr;
    if (!Root->TryGetArrayField(TEXT("endings"), Endings)) return false;
    Epilogues.Reset();
    for (const auto& Value : *Endings)
    {
        const auto Entry = Value->AsObject();
        if (Entry.IsValid())
        {
            const FName Id(*Entry->GetStringField(TEXT("id")));
            Epilogues.Add(Id, ReadText(*Entry, bItalian ? TEXT("epilogue") : TEXT("epilogue_en"), TEXT("epilogue")));
        }
    }
    Chapters = MoveTemp(Parsed);
    return true;
}

bool UEidolonCampaignSubsystem::NewCampaign()
{
    bFirstDescentReturnRequested = false;
    State = NewObject<UEidolonCampaignSave>(this);
    State->Flags.Add(TEXT("family_original"));
    return Chapters.Num() == 26;
}

bool UEidolonCampaignSubsystem::LoadCampaign()
{
    auto* Loaded = Cast<UEidolonCampaignSave>(UGameplayStatics::LoadGameFromSlot(CampaignSlot(), 0));
    if (!IsValidCheckpoint(Loaded)) return false;
    bFirstDescentReturnRequested = false;
    State = Loaded;
    return true;
}

bool UEidolonCampaignSubsystem::SaveCampaign()
{
    return WriteCheckpoint(State);
}

bool UEidolonCampaignSubsystem::WriteCheckpoint(UEidolonCampaignSave* Snapshot) const
{
    if (!IsValidCheckpoint(Snapshot)) return false;

    // Never replace an occupied slot whose data we cannot recover.
    const FString& Slot = CampaignSlot();
    if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
    {
        const auto* Existing = Cast<UEidolonCampaignSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
        if (!IsValidCheckpoint(Existing))
        {
            UE_LOG(LogTemp, Warning, TEXT("Checkpoint write blocked because the existing slot could not be validated."));
            return false;
        }
    }

    return UGameplayStatics::SaveGameToSlot(Snapshot, Slot, 0);
}

bool UEidolonCampaignSubsystem::IsValidCheckpoint(const UEidolonCampaignSave* Checkpoint) const
{
    if (!Checkpoint || Checkpoint->Schema != 1 || Checkpoint->Chapter <= 0 ||
        !Chapters.IsValidIndex(Checkpoint->Chapter - 1) || Checkpoint->ObjectiveCount < 0)
    {
        return false;
    }

    const EidolonAbyss::Progress Authored{
        Checkpoint->AuthoredAbyssCompletedFloor, Checkpoint->bAuthoredAbyssRicordaHeard
    };
    const int32 Cursor = Checkpoint->FirstDescentCursor;
    if (!EidolonAbyss::IsValid(Authored) ||
        (Authored.CompletedFloor > 0 && (Checkpoint->Chapter < EidolonAbyss::Chapter ||
            Cursor != FinalFirstDescentCursor)))
    {
        return false;
    }

    if (!EidolonSluice::MatchesFloorProgress(Checkpoint->AbyssSluiceProgress, Authored.CompletedFloor,
            EidolonAbyss::SluiceFloor) ||
        !EidolonSluice::MatchesFloorProgress(Checkpoint->AbyssFernBasinProgress, Authored.CompletedFloor,
            EidolonAbyss::FernBasinFloor))
    {
        return false;
    }

    using MarrowStage = EidolonMarrowLatch::Stage;
    const auto Marrow = static_cast<MarrowStage>(Checkpoint->AbyssMarrowLatchStage);
    if (!EidolonMarrowLatch::IsValid(Marrow) ||
        (Authored.CompletedFloor < EidolonAbyss::MarrowFloor - 1 && Marrow != MarrowStage::Locked) ||
        ((Marrow == MarrowStage::Open) != (Authored.CompletedFloor >= EidolonAbyss::MarrowFloor)))
    {
        return false;
    }

    const EidolonCounterweight::Progress Weight{
        static_cast<EidolonCounterweight::Location>(Checkpoint->AbyssCounterweightLocation),
        Checkpoint->AbyssCounterweightRevision
    };
    if (!EidolonCounterweight::IsValid(Weight) ||
        (Authored.CompletedFloor < EidolonAbyss::CounterweightFloor - 1 && Weight.Revision != 0) ||
        (EidolonCounterweight::CanExit(Weight) != (Authored.CompletedFloor >= EidolonAbyss::CounterweightFloor)))
    {
        return false;
    }

    const EidolonMirror::Progress Mirrors{
        Checkpoint->AbyssFirstMirrorOrientation, Checkpoint->AbyssSecondMirrorOrientation,
        Checkpoint->AbyssMirrorRevision
    };
    if (!EidolonMirror::IsValid(Mirrors) ||
        (Authored.CompletedFloor < EidolonAbyss::MirrorFloor - 1 && Mirrors.Revision != 0) ||
        (EidolonMirror::CanExit(Mirrors) != (Authored.CompletedFloor >= EidolonAbyss::MirrorFloor)))
    {
        return false;
    }

    const auto PincerWinch = static_cast<EidolonPincerWinch::Stage>(Checkpoint->AbyssPincerWinchStage);
    if (!EidolonPincerWinch::IsValid(PincerWinch) ||
        (Authored.CompletedFloor < EidolonAbyss::PincerWinchFloor - 1 &&
            PincerWinch != EidolonPincerWinch::Stage::Initial) ||
        (EidolonPincerWinch::CanExit(PincerWinch) != (Authored.CompletedFloor >= EidolonAbyss::PincerWinchFloor)))
    {
        return false;
    }

    const auto BranchLatch = static_cast<EidolonBranchLatch::Route>(Checkpoint->AbyssBranchLatchRoute);
    if (!EidolonBranchLatch::IsValid(BranchLatch) ||
        (EidolonBranchLatch::CanExit(BranchLatch) != (Authored.CompletedFloor >= EidolonAbyss::BranchLatchFloor)))
    {
        return false;
    }

    return Cursor == 0 ||
        (Cursor > 0 && Cursor < FinalFirstDescentCursor && Checkpoint->Chapter == 2) ||
        (Cursor == FinalFirstDescentCursor && Checkpoint->Chapter >= 3);
}

int32 UEidolonCampaignSubsystem::GetFirstDescentCursor() const
{
    return State ? State->FirstDescentCursor : 0;
}

int32 UEidolonCampaignSubsystem::GetAuthoredAbyssCompletedFloor() const
{
    return State ? ReadAuthoredAbyssProgress(*State).CompletedFloor : 0;
}

int32 UEidolonCampaignSubsystem::GetAuthoredAbyssRevision() const
{
    return State ? EidolonAbyss::Revision(ReadAuthoredAbyssProgress(*State)) : 0;
}

bool UEidolonCampaignSubsystem::CommitAuthoredAbyssEvent(const EidolonAbyss::Event& Event)
{
    if (!IsInGameThread() || !IsValidCheckpoint(State) || !State->Ending.IsNone() ||
        State->Chapter != EidolonAbyss::Chapter ||
        State->FirstDescentCursor != FinalFirstDescentCursor ||
        Event.Kind == EidolonAbyss::EventKind::SluicesOpened ||
        Event.Kind == EidolonAbyss::EventKind::FernBasinDrained ||
        Event.Kind == EidolonAbyss::EventKind::MarrowLatchOpened ||
        Event.Kind == EidolonAbyss::EventKind::CounterweightInstalled ||
        Event.Kind == EidolonAbyss::EventKind::MirrorsAligned ||
        Event.Kind == EidolonAbyss::EventKind::PincerWinchOpened ||
        Event.Kind == EidolonAbyss::EventKind::BranchLatchSelected)
    {
        return false;
    }

    const auto Next = EidolonAbyss::Advance(ReadAuthoredAbyssProgress(*State), Event);
    const UGameInstance* GameInstance = GetGameInstance();
    if (!Next || !GameInstance || !IsAuthoredAbyssMap(GameInstance->GetWorld(), Event.AuthenticatedFloor))
        return false;

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->AuthoredAbyssCompletedFloor = Next->CompletedFloor;
    Snapshot->bAuthoredAbyssRicordaHeard = Next->RicordaHeard;
    if (Event.Kind == EidolonAbyss::EventKind::RicordaHeard)
        Snapshot->Memories.Add(TEXT("abyss_ricorda"));
    if (Event.Kind == EidolonAbyss::EventKind::RootclawDefeated)
        Snapshot->Flags.Add(TEXT("abyss_rootclaw_defeated"));
    if (Event.Kind == EidolonAbyss::EventKind::GuardianDefeated)
    {
        Snapshot->AbyssFloor = EidolonAbyss::GuardianFloor;
        Snapshot->Flags.Add(TEXT("abyss_guardian_defeated"));
        const int32 Target = CurrentChapter().Target;
        if (Snapshot->ObjectiveCount < Target)
        {
            Snapshot->ObjectiveCount = Target;
            Snapshot->Experience = static_cast<int32>(FMath::Min<int64>(MAX_int32,
                static_cast<int64>(Snapshot->Experience) + 25));
        }
    }
    if (!WriteCheckpoint(Snapshot)) return false;

    State = Snapshot;
    return true;
}

uint8 UEidolonCampaignSubsystem::GetAbyssDrainProgress(int32 Floor) const
{
    const auto Member = DrainMaskMember(Floor);
    return State && Member ? State.Get()->*Member : 0;
}

bool UEidolonCampaignSubsystem::CommitAbyssDrain(int32 Floor, EidolonSluice::Control Control, uint8 ExpectedProgress)
{
    using namespace EidolonAbyss;
    const auto Member = DrainMaskMember(Floor);
    if (!Member || !IsInGameThread() || !IsValidCheckpoint(State) || !State->Ending.IsNone() ||
        State->Chapter != Chapter || State->FirstDescentCursor != FinalFirstDescentCursor ||
        GetAuthoredAbyssCompletedFloor() != Floor - 1)
    {
        return false;
    }

    const auto* GameInstance = GetGameInstance();
    if (!GameInstance || !IsAuthoredAbyssMap(GameInstance->GetWorld(), Floor)) return false;
    const auto NextSluice = EidolonSluice::ProposeActivation(State.Get()->*Member, ExpectedProgress, Control);
    if (!NextSluice) return false;

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->*Member = *NextSluice;
    if (EidolonSluice::CanExit(*NextSluice))
    {
        const auto Progress = ReadAuthoredAbyssProgress(*State);
        const bool bFernBasin = Floor == FernBasinFloor;
        const auto NextFloor = Advance(Progress, {Chapter, Floor, Floor, Revision(Progress),
            bFernBasin ? EventKind::FernBasinDrained : EventKind::SluicesOpened, Producer::Puzzle,
            bFernBasin ? Target::FernBasinDrains : Target::SluiceControls});
        if (!NextFloor) return false;
        Snapshot->AuthoredAbyssCompletedFloor = NextFloor->CompletedFloor;
    }
    if (!WriteCheckpoint(Snapshot)) return false;
    State = Snapshot;
    return true;
}

EidolonMarrowLatch::Stage UEidolonCampaignSubsystem::GetAbyssMarrowLatchStage() const
{
    return State ? static_cast<EidolonMarrowLatch::Stage>(State->AbyssMarrowLatchStage)
        : EidolonMarrowLatch::Stage::Locked;
}

bool UEidolonCampaignSubsystem::CommitAbyssMarrowLatch(EidolonMarrowLatch::Signal Signal, EidolonMarrowLatch::Stage ExpectedStage)
{
    using namespace EidolonAbyss;
    if (!IsInGameThread() || !IsValidCheckpoint(State) || !State->Ending.IsNone() ||
        State->Chapter != Chapter || State->FirstDescentCursor != FinalFirstDescentCursor ||
        GetAuthoredAbyssCompletedFloor() != MarrowFloor - 1)
    {
        return false;
    }

    const auto* GameInstance = GetGameInstance();
    if (!GameInstance || !IsAuthoredAbyssMap(GameInstance->GetWorld(), MarrowFloor)) return false;
    const auto NextLatch = EidolonMarrowLatch::Propose(GetAbyssMarrowLatchStage(), ExpectedStage, Signal);
    if (!NextLatch) return false;

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->AbyssMarrowLatchStage = static_cast<uint8>(*NextLatch);
    if (*NextLatch == EidolonMarrowLatch::Stage::Open)
    {
        const auto Progress = ReadAuthoredAbyssProgress(*State);
        const auto NextFloor = Advance(Progress, {Chapter, MarrowFloor, MarrowFloor, Revision(Progress),
            EventKind::MarrowLatchOpened, Producer::Puzzle, Target::MarrowLatch});
        if (!NextFloor) return false;
        Snapshot->AuthoredAbyssCompletedFloor = NextFloor->CompletedFloor;
    }
    if (!WriteCheckpoint(Snapshot)) return false;
    State = Snapshot;
    return true;
}

EidolonCounterweight::Progress UEidolonCampaignSubsystem::GetAbyssCounterweightProgress() const
{
    if (!State) return {};
    return {static_cast<EidolonCounterweight::Location>(State->AbyssCounterweightLocation),
        State->AbyssCounterweightRevision};
}

bool UEidolonCampaignSubsystem::CommitAbyssCounterweight(const EidolonCounterweight::Event& Event)
{
    using namespace EidolonAbyss;
    if (!IsInGameThread() || !IsValidCheckpoint(State) || !State->Ending.IsNone() ||
        State->Chapter != Chapter || State->FirstDescentCursor != FinalFirstDescentCursor ||
        GetAuthoredAbyssCompletedFloor() != CounterweightFloor - 1)
    {
        return false;
    }

    const auto* GameInstance = GetGameInstance();
    if (!GameInstance || !IsAuthoredAbyssMap(GameInstance->GetWorld(), CounterweightFloor)) return false;
    const auto NextWeight = EidolonCounterweight::Advance(GetAbyssCounterweightProgress(), Event);
    if (!NextWeight) return false;

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->AbyssCounterweightLocation = static_cast<uint8>(NextWeight->Weight);
    Snapshot->AbyssCounterweightRevision = NextWeight->Revision;
    if (EidolonCounterweight::CanExit(*NextWeight))
    {
        const auto Progress = ReadAuthoredAbyssProgress(*State);
        const auto NextFloor = Advance(Progress, {Chapter, CounterweightFloor, CounterweightFloor,
            Revision(Progress), EventKind::CounterweightInstalled, Producer::Puzzle, Target::CounterweightWinch});
        if (!NextFloor) return false;
        Snapshot->AuthoredAbyssCompletedFloor = NextFloor->CompletedFloor;
    }
    if (!WriteCheckpoint(Snapshot)) return false;
    State = Snapshot;
    return true;
}

EidolonMirror::Progress UEidolonCampaignSubsystem::GetAbyssMirrorProgress() const
{
    if (!State) return {};
    return {State->AbyssFirstMirrorOrientation, State->AbyssSecondMirrorOrientation,
        State->AbyssMirrorRevision};
}

bool UEidolonCampaignSubsystem::CommitAbyssMirror(const EidolonMirror::Event& Event)
{
    using namespace EidolonAbyss;
    if (!IsInGameThread() || !IsValidCheckpoint(State) || !State->Ending.IsNone() ||
        State->Chapter != Chapter || State->FirstDescentCursor != FinalFirstDescentCursor ||
        GetAuthoredAbyssCompletedFloor() != MirrorFloor - 1)
    {
        return false;
    }

    const auto* GameInstance = GetGameInstance();
    if (!GameInstance || !IsAuthoredAbyssMap(GameInstance->GetWorld(), MirrorFloor)) return false;
    const auto NextMirrors = EidolonMirror::Advance(GetAbyssMirrorProgress(), Event);
    if (!NextMirrors) return false;

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->AbyssFirstMirrorOrientation = NextMirrors->First;
    Snapshot->AbyssSecondMirrorOrientation = NextMirrors->Second;
    Snapshot->AbyssMirrorRevision = NextMirrors->Revision;
    if (EidolonMirror::CanExit(*NextMirrors))
    {
        const auto Progress = ReadAuthoredAbyssProgress(*State);
        const auto NextFloor = Advance(Progress, {Chapter, MirrorFloor, MirrorFloor, Revision(Progress),
            EventKind::MirrorsAligned, Producer::Puzzle, Target::MirrorControls});
        if (!NextFloor) return false;
        Snapshot->AuthoredAbyssCompletedFloor = NextFloor->CompletedFloor;
    }
    if (!WriteCheckpoint(Snapshot)) return false;
    State = Snapshot;
    return true;
}

EidolonPincerWinch::Stage UEidolonCampaignSubsystem::GetAbyssPincerWinchStage() const
{
    return State ? static_cast<EidolonPincerWinch::Stage>(State->AbyssPincerWinchStage)
        : EidolonPincerWinch::Stage::Initial;
}

bool UEidolonCampaignSubsystem::CommitAbyssPincerWinch(EidolonPincerWinch::Signal Signal,
                                                       EidolonPincerWinch::Stage ExpectedStage)
{
    using namespace EidolonAbyss;
    if (!IsInGameThread() || !IsValidCheckpoint(State) || !State->Ending.IsNone() ||
        State->Chapter != Chapter || State->FirstDescentCursor != FinalFirstDescentCursor ||
        GetAuthoredAbyssCompletedFloor() != PincerWinchFloor - 1)
    {
        return false;
    }

    const auto* GameInstance = GetGameInstance();
    if (!GameInstance || !IsAuthoredAbyssMap(GameInstance->GetWorld(), PincerWinchFloor)) return false;
    const auto NextWinch = EidolonPincerWinch::Propose(GetAbyssPincerWinchStage(), ExpectedStage, Signal);
    if (!NextWinch) return false;

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->AbyssPincerWinchStage = static_cast<uint8>(*NextWinch);
    if (EidolonPincerWinch::CanExit(*NextWinch))
    {
        const auto Progress = ReadAuthoredAbyssProgress(*State);
        const auto NextFloor = Advance(Progress, {Chapter, PincerWinchFloor, PincerWinchFloor, Revision(Progress),
            EventKind::PincerWinchOpened, Producer::Puzzle, Target::PincerWinch});
        if (!NextFloor) return false;
        Snapshot->AuthoredAbyssCompletedFloor = NextFloor->CompletedFloor;
    }
    if (!WriteCheckpoint(Snapshot)) return false;
    State = Snapshot;
    return true;
}

EidolonBranchLatch::Route UEidolonCampaignSubsystem::GetAbyssBranchLatchRoute() const
{
    return State ? static_cast<EidolonBranchLatch::Route>(State->AbyssBranchLatchRoute)
        : EidolonBranchLatch::Route::Unselected;
}

bool UEidolonCampaignSubsystem::CommitAbyssBranchLatch(EidolonBranchLatch::Route VerifiedLatch,
                                                      EidolonBranchLatch::Route ExpectedRoute)
{
    using namespace EidolonAbyss;
    if (!IsInGameThread() || !IsValidCheckpoint(State) || !State->Ending.IsNone() ||
        State->Chapter != Chapter || State->FirstDescentCursor != FinalFirstDescentCursor ||
        GetAuthoredAbyssCompletedFloor() != BranchLatchFloor - 1)
    {
        return false;
    }

    const auto* GameInstance = GetGameInstance();
    if (!GameInstance || !IsAuthoredAbyssMap(GameInstance->GetWorld(), BranchLatchFloor)) return false;
    const auto NextRoute = EidolonBranchLatch::Propose(GetAbyssBranchLatchRoute(), ExpectedRoute, VerifiedLatch);
    if (!NextRoute) return false;

    const auto Progress = ReadAuthoredAbyssProgress(*State);
    const auto NextFloor = Advance(Progress, {Chapter, BranchLatchFloor, BranchLatchFloor, Revision(Progress),
        EventKind::BranchLatchSelected, Producer::Puzzle, Target::BranchLatches});
    if (!NextFloor) return false;

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->AbyssBranchLatchRoute = static_cast<uint8>(*NextRoute);
    Snapshot->AuthoredAbyssCompletedFloor = NextFloor->CompletedFloor;
    if (!WriteCheckpoint(Snapshot)) return false;
    State = Snapshot;
    return true;
}

bool UEidolonCampaignSubsystem::CanEnterWorldRegion(FName RegionId) const
{
    EidolonWorld::Region Destination;
    if (!State || !ResolveRegion(RegionId, Destination)) return false;

    const EidolonWorld::Progress Progress{
        State->Chapter, State->GiantsDefeated, State->StairFloor,
        HasFlag(TEXT("sigil_aegis")), HasFlag(TEXT("sigil_horizon"))
    };
    return EidolonWorld::CanEnter(Destination, Progress);
}

bool UEidolonCampaignSubsystem::CommitFirstDescentEvent(const EidolonFirstDescent::Event& Event)
{
    if (!IsInGameThread() || !IsValidCheckpoint(State) ||
        State->Chapter != Event.Chapter || !State->Ending.IsNone())
    {
        return false;
    }

    const auto Next = EidolonFirstDescent::Advance(State->FirstDescentCursor, Event);
    if (!Next) return false;
    if (Event.Payload.Event == "surface_return" &&
        (!bFirstDescentReturnRequested || UGameplayStatics::GetCurrentLevelName(GetGameInstance()) != TEXT("ValdrisPrologue")))
    {
        return false;
    }

    // The cursor is the durable receipt because each position has one accepted payload.
    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->FirstDescentCursor = static_cast<int32>(*Next);
    if (*Next == EidolonFirstDescent::Steps.size())
    {
        RevealNoah(*Snapshot);
        Snapshot->Flags.Add(TEXT("chapter_2_resolved"));
        Snapshot->Chapter = 3;
        Snapshot->ObjectiveCount = 0;
    }
    if (!WriteCheckpoint(Snapshot)) return false;

    State = Snapshot;
    if (Event.Payload.Event == "surface_return") bFirstDescentReturnRequested = false;
    return true;
}

bool UEidolonCampaignSubsystem::RequestFirstDescentSurfaceReturn()
{
    if (!State || State->Chapter != 2 || State->FirstDescentCursor != 4 ||
        UGameplayStatics::GetCurrentLevelName(GetGameInstance()) != TEXT("AbyssFloor001"))
    {
        return false;
    }
    bFirstDescentReturnRequested = true;
    return true;
}

bool UEidolonCampaignSubsystem::BeginValdrisFirstDescent()
{
    if (!IsValidCheckpoint(State) || State->Chapter != 1 || !State->Ending.IsNone() ||
        UGameplayStatics::GetCurrentLevelName(GetGameInstance()) != TEXT("ValdrisPrologue"))
    {
        return false;
    }

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->Flags.Add(TEXT("chapter_1_resolved"));
    Snapshot->Chapter = 2;
    Snapshot->ObjectiveCount = 0;
    if (!WriteCheckpoint(Snapshot)) return false;
    State = Snapshot;
    return true;
}

bool UEidolonCampaignSubsystem::CompleteFirstDescentSurfaceReturn()
{
    return bFirstDescentReturnRequested &&
        CommitFirstDescentEvent({2, "mq_02", 4, EidolonFirstDescent::Steps[4]});
}

bool UEidolonCampaignSubsystem::CommitRivenConversation()
{
    if (!IsValidCheckpoint(State) || State->Chapter != 3 || !State->Ending.IsNone() ||
        State->FirstDescentCursor != static_cast<int32>(EidolonFirstDescent::Steps.size()) ||
        HasFlag(TEXT("chapter_3_riven_spoken")) ||
        UGameplayStatics::GetCurrentLevelName(GetGameInstance()) != TEXT("AbyssFloor001"))
    {
        return false;
    }

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->Flags.Add(TEXT("chapter_3_riven_spoken"));
    if (!WriteCheckpoint(Snapshot)) return false;
    State = Snapshot;
    return true;
}

bool UEidolonCampaignSubsystem::FinishSecondDay()
{
    if (!IsValidCheckpoint(State) || State->Chapter != 3 || !State->Ending.IsNone() ||
        !HasFlag(TEXT("chapter_3_riven_spoken")) ||
        UGameplayStatics::GetCurrentLevelName(GetGameInstance()) != TEXT("ValdrisPrologue"))
    {
        return false;
    }

    UEidolonCampaignSave* Snapshot = DuplicateObject<UEidolonCampaignSave>(State.Get(), this);
    Snapshot->Flags.Add(TEXT("chapter_3_resolved"));
    Snapshot->Chapter = 4;
    Snapshot->ObjectiveCount = 0;
    if (!WriteCheckpoint(Snapshot)) return false;
    State = Snapshot;
    return true;
}

FEidolonChapter UEidolonCampaignSubsystem::CurrentChapter() const
{
    return State && Chapters.IsValidIndex(State->Chapter-1) ? Chapters[State->Chapter-1] : FEidolonChapter();
}
int32 UEidolonCampaignSubsystem::GetObjectiveCount() const { return State ? State->ObjectiveCount : 0; }
int32 UEidolonCampaignSubsystem::GetFloor(bool bStair) const { return State ? (bStair ? State->StairFloor : State->AbyssFloor) : 0; }
FName UEidolonCampaignSubsystem::GetEnding() const { return State ? State->Ending : NAME_None; }
FString UEidolonCampaignSubsystem::GetEpilogue() const { return Epilogues.FindRef(GetEnding()); }
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
    if (C.Id == 2)
    {
        if (State->FirstDescentCursor != 0) return false;
        RevealNoah(*State);
    }
    if (C.Id == 4) SetFlag(TEXT("sigil_aegis"));
    if (C.Id == 7) SetFlag(TEXT("sigil_horizon"));
    if (C.Id == 10) ApplyDiscrepancy(TEXT("vareth_survivor"), TEXT("vareth_present"), false);
    if (C.Id == 22 && !HasFlag(TEXT("noah_found_in_oblivion"))) return false;
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
