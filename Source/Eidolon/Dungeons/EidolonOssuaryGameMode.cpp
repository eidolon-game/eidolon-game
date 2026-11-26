#include "EidolonOssuaryGameMode.h"

#include "EidolonAbyssProgression.h"
#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "EidolonOssuaryTick.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace
{
    const FName OssuaryMap(TEXT("/Game/Eidolon/Maps/AbyssFloor002"));
    const FString NextMap(TEXT("/Game/Eidolon/Maps/AbyssFloor003"));
    const TCHAR* CheckpointOption = TEXT("OssuaryCheckpoint");
    constexpr int32 Floor = 2;
    constexpr double InteractionRange = 180.0;
    constexpr double TickSpawnHeight = 30.0;

    const TCHAR* OssuaryText(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName()
            == TEXT("it") ? Italian : English;
    }

    template <typename ActorType>
    ActorType* FindTaggedActor(UWorld& World, FName Tag)
    {
        ActorType* Found = nullptr;
        for (TActorIterator<AActor> It(&World); It; ++It)
        {
            if (It->IsActorBeingDestroyed() || !It->ActorHasTag(Tag)) continue;
            if (Found || !It->HasAuthority()) return nullptr;
            Found = Cast<ActorType>(*It);
            if (!Found) return nullptr;
        }
        return Found;
    }

    bool CanUseFloor(const UEidolonCampaignSubsystem& Campaign)
    {
        return Campaign.CurrentChapter().Id == EidolonAbyss::Chapter && Campaign.GetEnding().IsNone()
            && Campaign.GetAuthoredAbyssCompletedFloor() >= Floor - 1;
    }
}

void AEidolonOssuaryGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    bFloorBound = BindFloor();
    if (!bFloorBound)
    {
        Notice = OssuaryText(TEXT("The Ossuary layout is incomplete."), TEXT("La mappa dell'Ossario è incompleta."));
        UE_LOG(LogTemp, Error, TEXT("AbyssFloor002 lacks unique authored bindings."));
        return;
    }

    const auto* Campaign = GetCampaign();
    SetGateOpen(Campaign && CanUseFloor(*Campaign) && Campaign->GetAuthoredAbyssCompletedFloor() >= Floor);
}

AActor* AEidolonOssuaryGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    auto* World = GetWorld();
    const bool bCheckpoint = UGameplayStatics::HasOption(OptionsString, CheckpointOption);
    const FName Tag = bCheckpoint ? TEXT("abyss_002_checkpoint_start") : TEXT("abyss_002_player_start");
    if (World)
    {
        if (auto* Start = FindTaggedActor<APlayerStart>(*World, Tag)) return Start;
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

UEidolonCampaignSubsystem* AEidolonOssuaryGameMode::GetCampaign() const
{
    auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

bool AEidolonOssuaryGameMode::BindFloor()
{
    auto* World = GetWorld();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != TEXT("AbyssFloor002")
        || FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
    {
        return false;
    }

    Arena = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_002_encounter"));
    TickSpawn = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_002_ossuary_tick_01"));
    Exit = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_002_exit"));
    Gate = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_002_ossuary_gate"));
    return Arena.IsValid() && TickSpawn.IsValid() && Exit.IsValid() && Gate.IsValid()
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_002_player_start"))
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_002_checkpoint_start"));
}

bool AEidolonOssuaryGameMode::CanInteractAt(const AEidolonCharacter& Player, const ATargetPoint* Marker) const
{
    auto* World = GetWorld();
    const auto* Controller = World ? World->GetFirstPlayerController() : nullptr;
    return IsValid(Marker) && Controller && Controller->GetPawn() == &Player && !Player.IsDefeated()
        && EidolonQuest::CanReachInteractionTarget(Player, *Marker, InteractionRange);
}

void AEidolonOssuaryGameMode::Interact(AEidolonCharacter* Player)
{
    auto* Campaign = GetCampaign();
    if (!bFloorBound || !Gate.IsValid() || !IsValid(Player) || !Campaign || !CanUseFloor(*Campaign))
        return;

    if (Campaign->GetAuthoredAbyssCompletedFloor() >= Floor)
    {
        if (!CanInteractAt(*Player, Exit.Get())) return;
        if (!FPackageName::DoesPackageExist(NextMap))
        {
            Notice = OssuaryText(TEXT("The route below is unavailable."), TEXT("Il passaggio verso il livello inferiore non è disponibile."));
            return;
        }
        UGameplayStatics::OpenLevel(this, FName(*NextMap));
        return;
    }

    if (!CanInteractAt(*Player, Arena.Get())) return;
    if (bDefeatAwaitingReceipt)
        CommitEncounter(*Campaign);
    else if (!IsValid(ActiveTick.Get()))
        SpawnTick(*Campaign);
}

void AEidolonOssuaryGameMode::SpawnTick(UEidolonCampaignSubsystem& Campaign)
{
    const auto* Marker = TickSpawn.Get();
    if (!Marker) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    ActiveTick = GetWorld()->SpawnActor<AEidolonOssuaryTick>(
        Marker->GetActorLocation() + FVector(0, 0, TickSpawnHeight), Marker->GetActorRotation(), Params);
    if (!IsValid(ActiveTick.Get()))
    {
        Notice = OssuaryText(TEXT("Clear the niche and interact to retry."), TEXT("Libera la nicchia e interagisci per riprovare."));
        return;
    }

    SpawnRevision = Campaign.GetAuthoredAbyssRevision();
    ActiveTick->OnDefeated.AddUObject(this, &AEidolonOssuaryGameMode::HandleDefeat);
    Notice.Reset();
}

void AEidolonOssuaryGameMode::HandleDefeat(AEidolonOssuaryTick& Tick)
{
    if (ActiveTick.Get() != &Tick || !bFloorBound || !Gate.IsValid()) return;
    ActiveTick = nullptr;
    bDefeatAwaitingReceipt = true;
    if (auto* Campaign = GetCampaign()) CommitEncounter(*Campaign);
}

void AEidolonOssuaryGameMode::CommitEncounter(UEidolonCampaignSubsystem& Campaign)
{
    using namespace EidolonAbyss;
    const Event Defeat{Chapter, Floor, Floor, SpawnRevision,
                       EventKind::FloorCleared, Producer::Encounter, Target::FloorEncounter};
    if (!bDefeatAwaitingReceipt || !Campaign.CommitAuthoredAbyssEvent(Defeat))
    {
        Notice = OssuaryText(TEXT("Progress could not be saved. Interact at the niche to retry."),
                             TEXT("Impossibile salvare i progressi. Interagisci alla nicchia per riprovare."));
        return;
    }

    bDefeatAwaitingReceipt = false;
    SpawnRevision = INDEX_NONE;
    Notice.Reset();
    SetGateOpen(true);
}

void AEidolonOssuaryGameMode::SetGateOpen(bool bOpen)
{
    if (auto* Door = Gate.Get())
    {
        Door->SetActorEnableCollision(!bOpen);
        Door->SetActorHiddenInGame(bOpen);
    }
}

void AEidolonOssuaryGameMode::ReloadCheckpoint()
{
    if (!bFloorBound) return;
    if (auto* Campaign = GetCampaign())
    {
        if (!Campaign->LoadCampaign())
        {
            UE_LOG(LogTemp, Warning, TEXT("Ossuary checkpoint could not be read. Preserving current state."));
        }
        if (CanUseFloor(*Campaign)) UGameplayStatics::OpenLevel(this, OssuaryMap, true, CheckpointOption);
    }
}

FString AEidolonOssuaryGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    const auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign))
        return OssuaryText(TEXT("Enter after completing the previous descent."), TEXT("Entra dopo aver completato la discesa precedente."));
    if (Campaign->GetAuthoredAbyssCompletedFloor() >= Floor)
        return OssuaryText(TEXT("Interact at the stairs down."), TEXT("Interagisci alla scala verso il basso."));
    if (IsValid(ActiveTick.Get()))
        return OssuaryText(TEXT("Evade the leap, then attack during recovery."), TEXT("Schiva il salto e attacca durante il recupero."));
    return OssuaryText(TEXT("E / X or Square at the niche."), TEXT("E / X o Quadrato alla nicchia."));
}

FString AEidolonOssuaryGameMode::GetNarrative() const
{
    return OssuaryText(TEXT("Low-Arch Ossuary | Abyss, floor 2"), TEXT("Ossario degli Archi Bassi | Abisso, livello 2"));
}
