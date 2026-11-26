#include "EidolonAuthoredAbyssGameMode.h"

#include "EidolonAbyssProgression.h"
#include "../EidolonArenaEnemy.h"
#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
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
#include "Misc/Paths.h"
#include "UObject/Package.h"

namespace
{
    constexpr double InteractionRange = 180.0;
    const TCHAR* CheckpointOption = TEXT("AbyssCheckpoint");
    const FString MapFolder = TEXT("/Game/Eidolon/Maps");

    FString FloorName(int32 Floor)
    {
        return FString::Printf(TEXT("AbyssFloor%03d"), Floor);
    }

    FName FloorTag(int32 Floor, const TCHAR* Suffix)
    {
        return FName(*FString::Printf(TEXT("abyss_%03d_%s"), Floor, Suffix));
    }

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
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
}

void AEidolonAuthoredAbyssGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    bFloorBound = BindFloor();
    if (!bFloorBound)
    {
        Notice = Display(TEXT("This Abyss floor is incomplete."), TEXT("Questo piano dell'Abisso è incompleto."));
        UE_LOG(LogTemp, Error, TEXT("%s lacks unique authored bindings."), *FloorName(GetEncounter().Floor));
        return;
    }

    const auto* Campaign = GetCampaign();
    SetGateOpen(Campaign && CanUseFloor(*Campaign) &&
                Campaign->GetAuthoredAbyssCompletedFloor() >= GetEncounter().Floor);
}

AActor* AEidolonAuthoredAbyssGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* World = GetWorld())
    {
        const bool bCheckpoint = UGameplayStatics::HasOption(OptionsString, CheckpointOption);
        const FName Tag = FloorTag(GetEncounter().Floor, bCheckpoint ? TEXT("checkpoint_start") : TEXT("player_start"));
        if (auto* Start = FindTaggedActor<APlayerStart>(*World, Tag)) return Start;
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

UEidolonCampaignSubsystem* AEidolonAuthoredAbyssGameMode::GetCampaign() const
{
    auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

bool AEidolonAuthoredAbyssGameMode::CanUseFloor(const UEidolonCampaignSubsystem& Campaign) const
{
    return Campaign.CurrentChapter().Id == EidolonAbyss::Chapter && Campaign.GetEnding().IsNone()
        && Campaign.GetAuthoredAbyssCompletedFloor() >= GetEncounter().Floor - 1;
}

bool AEidolonAuthoredAbyssGameMode::BindFloor()
{
    auto* World = GetWorld();
    const auto Encounter = GetEncounter();
    if (!World || Encounter.Floor < 2 || !Encounter.EnemyClass
        || UGameplayStatics::GetCurrentLevelName(this) != FloorName(Encounter.Floor)
        || FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != MapFolder)
    {
        return false;
    }

    Arena = FindTaggedActor<ATargetPoint>(*World, FloorTag(Encounter.Floor, TEXT("encounter")));
    EnemySpawn = FindTaggedActor<ATargetPoint>(*World, Encounter.SpawnTag);
    Exit = FindTaggedActor<ATargetPoint>(*World, FloorTag(Encounter.Floor, TEXT("exit")));
    Gate = FindTaggedActor<AStaticMeshActor>(*World, Encounter.GateTag);
    if (Encounter.bRequiresLever)
        Lever = FindTaggedActor<ATargetPoint>(*World, FloorTag(Encounter.Floor, TEXT("lever")));

    return Arena.IsValid() && EnemySpawn.IsValid() && Exit.IsValid() && Gate.IsValid()
        && (!Encounter.bRequiresLever || Lever.IsValid())
        && FindTaggedActor<APlayerStart>(*World, FloorTag(Encounter.Floor, TEXT("player_start")))
        && FindTaggedActor<APlayerStart>(*World, FloorTag(Encounter.Floor, TEXT("checkpoint_start")));
}

bool AEidolonAuthoredAbyssGameMode::CanInteractAt(const AEidolonCharacter& Player, const ATargetPoint* Marker) const
{
    auto* World = GetWorld();
    const auto* Controller = World ? World->GetFirstPlayerController() : nullptr;
    return IsValid(Marker) && Controller && Controller->GetPawn() == &Player && !Player.IsDefeated()
        && EidolonQuest::CanReachInteractionTarget(Player, *Marker, InteractionRange);
}

void AEidolonAuthoredAbyssGameMode::Interact(AEidolonCharacter* Player)
{
    auto* Campaign = GetCampaign();
    if (!bFloorBound || !Gate.IsValid() || !IsValid(Player) || !Campaign || !CanUseFloor(*Campaign))
        return;

    const auto Encounter = GetEncounter();
    if (Campaign->GetAuthoredAbyssCompletedFloor() >= Encounter.Floor)
    {
        if (!CanInteractAt(*Player, Exit.Get())) return;
        const FString Destination = FPaths::Combine(MapFolder, FloorName(Encounter.Floor + 1));
        if (!FPackageName::DoesPackageExist(Destination))
        {
            Notice = Display(TEXT("The route below is unavailable."), TEXT("Il passaggio verso il livello inferiore non è disponibile."));
            return;
        }
        UGameplayStatics::OpenLevel(this, FName(*Destination));
        return;
    }

    if (bDefeatAwaitingReceipt)
    {
        const auto* ReceiptAnchor = Encounter.bRequiresLever ? Lever.Get() : Arena.Get();
        if (CanInteractAt(*Player, ReceiptAnchor)) CommitEncounter(*Campaign);
        return;
    }

    if (!IsValid(ActiveEnemy.Get()) && CanInteractAt(*Player, Arena.Get()))
        SpawnEnemy(*Campaign);
}

void AEidolonAuthoredAbyssGameMode::SpawnEnemy(UEidolonCampaignSubsystem& Campaign)
{
    const auto* Marker = EnemySpawn.Get();
    if (!Marker) return;

    const auto Encounter = GetEncounter();
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    ActiveEnemy = GetWorld()->SpawnActor<AEidolonArenaEnemy>(
        Encounter.EnemyClass, Marker->GetActorLocation() + FVector(0, 0, Encounter.SpawnHeight),
        Marker->GetActorRotation(), Params);
    if (!IsValid(ActiveEnemy.Get()))
    {
        Notice = Display(TEXT("Clear the spawn area and interact to retry."),
                         TEXT("Libera il punto di comparsa e interagisci per riprovare."));
        return;
    }

    SpawnRevision = Campaign.GetAuthoredAbyssRevision();
    ActiveEnemy->OnEncounterDefeated.AddUObject(this, &AEidolonAuthoredAbyssGameMode::HandleDefeat);
    Notice.Reset();
}

void AEidolonAuthoredAbyssGameMode::HandleDefeat(AEidolonArenaEnemy& Enemy)
{
    if (ActiveEnemy.Get() != &Enemy || !bFloorBound || !Gate.IsValid()) return;
    ActiveEnemy = nullptr;
    bDefeatAwaitingReceipt = true;
    if (!GetEncounter().bRequiresLever)
    {
        if (auto* Campaign = GetCampaign()) CommitEncounter(*Campaign);
    }
}

void AEidolonAuthoredAbyssGameMode::CommitEncounter(UEidolonCampaignSubsystem& Campaign)
{
    using namespace EidolonAbyss;
    const int32 Floor = GetEncounter().Floor;
    const Event Defeat{Chapter, Floor, Floor, SpawnRevision,
                       EventKind::FloorCleared, Producer::Encounter, Target::FloorEncounter};
    if (!bDefeatAwaitingReceipt || !Campaign.CommitAuthoredAbyssEvent(Defeat))
    {
        Notice = Display(TEXT("Progress could not be saved. Interact to retry."),
                         TEXT("Impossibile salvare i progressi. Interagisci per riprovare."));
        return;
    }

    bDefeatAwaitingReceipt = false;
    SpawnRevision = INDEX_NONE;
    Notice.Reset();
    SetGateOpen(true);
}

void AEidolonAuthoredAbyssGameMode::SetGateOpen(bool bOpen)
{
    if (auto* Door = Gate.Get())
    {
        Door->SetActorEnableCollision(!bOpen);
        Door->SetActorHiddenInGame(bOpen);
    }
}

void AEidolonAuthoredAbyssGameMode::ReloadCheckpoint()
{
    if (!bFloorBound) return;
    if (auto* Campaign = GetCampaign())
    {
        if (!Campaign->LoadCampaign())
        {
            UE_LOG(LogTemp, Warning, TEXT("Abyss checkpoint could not be read. Preserving current state."));
        }
        if (CanUseFloor(*Campaign))
        {
            const FString Destination = FPaths::Combine(MapFolder, FloorName(GetEncounter().Floor));
            UGameplayStatics::OpenLevel(this, FName(*Destination), true, CheckpointOption);
        }
    }
}

FString AEidolonAuthoredAbyssGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    const auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign))
        return Display(TEXT("Enter after completing the previous descent."), TEXT("Entra dopo aver completato la discesa precedente."));

    const auto Encounter = GetEncounter();
    if (Campaign->GetAuthoredAbyssCompletedFloor() >= Encounter.Floor)
        return Display(TEXT("Interact at the stairs down."), TEXT("Interagisci alla scala verso il basso."));
    if (bDefeatAwaitingReceipt && Encounter.bRequiresLever)
        return Display(TEXT("Pull the lever by the stair."), TEXT("Aziona la leva accanto alla scala."));
    if (IsValid(ActiveEnemy.Get()))
        return Display(Encounter.FightEnglish, Encounter.FightItalian);
    return Display(TEXT("Interact at the encounter marker."), TEXT("Interagisci nel punto dell'incontro."));
}

FString AEidolonAuthoredAbyssGameMode::GetNarrative() const
{
    const auto Encounter = GetEncounter();
    return Display(Encounter.TitleEnglish, Encounter.TitleItalian);
}
