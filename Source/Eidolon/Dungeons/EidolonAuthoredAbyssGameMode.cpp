#include "EidolonAuthoredAbyssGameMode.h"

#include "EidolonAbyssProgression.h"
#include "EidolonAbyssActorBindings.h"
#include "../EidolonArenaEnemy.h"
#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
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
    const bool bCanUseFloor = Campaign && CanUseFloor(*Campaign);
    const bool bCompleted = bCanUseFloor && Campaign->GetAuthoredAbyssCompletedFloor() >= GetEncounter().Floor;
    SetGateOpen(bCompleted);
    if (bCanUseFloor && !bCompleted && !GetEncounter().IsTraversal()) SpawnOptionalThreats();
}

AActor* AEidolonAuthoredAbyssGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* World = GetWorld())
    {
        const bool bCheckpoint = UGameplayStatics::HasOption(OptionsString, CheckpointOption);
        const FName Tag = FloorTag(GetEncounter().Floor, bCheckpoint ? TEXT("checkpoint_start") : TEXT("player_start"));
        if (auto* Start = EidolonAbyss::FindTaggedActor<APlayerStart>(*World, Tag)) return Start;
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
    if (!World || Encounter.Floor < 2
        || UGameplayStatics::GetCurrentLevelName(this) != FloorName(Encounter.Floor)
        || FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != MapFolder)
    {
        return false;
    }

    Exit = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, FloorTag(Encounter.Floor, TEXT("exit")));
    if (!Exit.IsValid()
        || !EidolonAbyss::FindTaggedActor<APlayerStart>(*World, FloorTag(Encounter.Floor, TEXT("player_start")))
        || !EidolonAbyss::FindTaggedActor<APlayerStart>(*World, FloorTag(Encounter.Floor, TEXT("checkpoint_start"))))
        return false;
    if (Encounter.IsTraversal()) return true;

    Gate = EidolonAbyss::FindTaggedActor<AStaticMeshActor>(*World, Encounter.GateTag);
    if (!Gate.IsValid()) return false;
    if (!Encounter.CompletionControlTag.IsNone())
    {
        Lever = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, Encounter.CompletionControlTag);
        if (!Encounter.ShortcutBlockTag.IsNone())
            ShortcutBlock = EidolonAbyss::FindTaggedActor<AStaticMeshActor>(*World, Encounter.ShortcutBlockTag);
        return Lever.IsValid() && (Encounter.ShortcutBlockTag.IsNone() || ShortcutBlock.IsValid())
            && BindCompletionActors(*World);
    }
    if (!Encounter.EnemyClass) return false;
    Arena = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, FloorTag(Encounter.Floor, TEXT("encounter")));
    EnemySpawn = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, Encounter.SpawnTag);
    if (Encounter.bRequiresLever)
        Lever = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, FloorTag(Encounter.Floor, TEXT("lever")));

    return Arena.IsValid() && EnemySpawn.IsValid()
        && (!Encounter.bRequiresLever || Lever.IsValid());
}

bool AEidolonAuthoredAbyssGameMode::CanInteractAt(const AEidolonCharacter& Player, const AActor* Marker) const
{
    auto* World = GetWorld();
    const auto* Controller = World ? World->GetFirstPlayerController() : nullptr;
    return IsValid(Marker) && Controller && Controller->GetPawn() == &Player && !Player.IsDefeated()
        && EidolonQuest::CanReachInteractionTarget(Player, *Marker, InteractionRange);
}

bool AEidolonAuthoredAbyssGameMode::InteractWithControl(AEidolonCharacter& Player, const ATargetPoint& Control)
{
    return CanInteractAt(Player, &Control);
}

void AEidolonAuthoredAbyssGameMode::Interact(AEidolonCharacter* Player)
{
    using namespace EidolonAbyss;
    auto* Campaign = GetCampaign();
    if (!bFloorBound || !IsValid(Player) || !Campaign || !CanUseFloor(*Campaign))
        return;

    const auto Encounter = GetEncounter();
    if (!Encounter.IsTraversal() && !Gate.IsValid()) return;

    const bool bCompleted = Campaign->GetAuthoredAbyssCompletedFloor() >= Encounter.Floor;
    if (bCompleted || Encounter.IsTraversal())
    {
        if (!CanInteractAt(*Player, Exit.Get())) return;
        if (!bCompleted && !SaveFloorCompletion(*Campaign, Producer::Traversal, Campaign->GetAuthoredAbyssRevision()))
            return;
        const FString Destination = FPaths::Combine(MapFolder, FloorName(Encounter.Floor + 1));
        if (!FPackageName::DoesPackageExist(Destination))
        {
            Notice = Display(TEXT("The route below is unavailable."), TEXT("Il passaggio verso il livello inferiore non è disponibile."));
            return;
        }
        UGameplayStatics::OpenLevel(this, FName(*Destination));
        return;
    }

    if (!Encounter.CompletionControlTag.IsNone())
    {
        const auto* Control = Lever.Get();
        if (Control && (Encounter.ShortcutBlockTag.IsNone() || ShortcutBlock.IsValid()) &&
            InteractWithControl(*Player, *Control) &&
            SaveFloorCompletion(*Campaign, Producer::Puzzle, Campaign->GetAuthoredAbyssRevision()))
        {
            SetGateOpen(true);
        }
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

    if (!ConfigureEnemy(*ActiveEnemy))
    {
        ActiveEnemy->Destroy();
        ActiveEnemy = nullptr;
        Notice = Display(TEXT("Encounter positions are unavailable."),
                         TEXT("Le posizioni dell'incontro non sono disponibili."));
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

bool AEidolonAuthoredAbyssGameMode::SaveFloorCompletion(UEidolonCampaignSubsystem& Campaign,
    EidolonAbyss::Producer Source, int32 Revision)
{
    using namespace EidolonAbyss;
    const auto Encounter = GetEncounter();
    const Event Completion{Chapter, Encounter.Floor, Encounter.Floor, Revision,
                           Encounter.CompletionKind, Source, Encounter.CompletionTarget};
    if (!Campaign.CommitAuthoredAbyssEvent(Completion))
    {
        Notice = Display(TEXT("Progress could not be saved. Interact to retry."),
                         TEXT("Impossibile salvare i progressi. Interagisci per riprovare."));
        return false;
    }

    Notice.Reset();
    return true;
}

void AEidolonAuthoredAbyssGameMode::CommitEncounter(UEidolonCampaignSubsystem& Campaign)
{
    if (!bDefeatAwaitingReceipt || !SaveFloorCompletion(Campaign, EidolonAbyss::Producer::Encounter, SpawnRevision))
        return;
    bDefeatAwaitingReceipt = false;
    SpawnRevision = INDEX_NONE;
    SetGateOpen(true);
}

void AEidolonAuthoredAbyssGameMode::SetGateOpen(bool bOpen)
{
    if (auto* Door = Gate.Get())
    {
        Door->SetActorEnableCollision(!bOpen);
        Door->SetActorHiddenInGame(bOpen);
    }
    if (auto* Block = ShortcutBlock.Get())
    {
        Block->SetActorEnableCollision(!bOpen);
        Block->SetActorHiddenInGame(bOpen);
    }
    UpdateCompletionActors(bOpen);
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
    if (Encounter.IsTraversal() || !Encounter.CompletionControlTag.IsNone()) return GetControlPrompt();
    if (bDefeatAwaitingReceipt && Encounter.bRequiresLever)
        return Display(TEXT("Pull the lever by the stair."), TEXT("Aziona la leva accanto alla scala."));
    if (IsValid(ActiveEnemy.Get()))
        return Display(Encounter.FightEnglish, Encounter.FightItalian);
    return Display(TEXT("Interact at the encounter marker."), TEXT("Interagisci nel punto dell'incontro."));
}

FString AEidolonAuthoredAbyssGameMode::GetControlPrompt() const
{
    const auto Encounter = GetEncounter();
    return Display(Encounter.FightEnglish, Encounter.FightItalian);
}

FString AEidolonAuthoredAbyssGameMode::GetNarrative() const
{
    const auto Encounter = GetEncounter();
    return Display(Encounter.TitleEnglish, Encounter.TitleItalian);
}
