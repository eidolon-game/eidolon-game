#include "EidolonMarrowRamGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonAbyssProgression.h"
#include "EidolonMarrowRam.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/PlayerStart.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace
{
    using EidolonMarrowLatch::Signal;
    using EidolonMarrowLatch::Stage;

    constexpr double RangeCm = 180.0;
    constexpr double RamSpawnHeightCm = 58.0;
    const FName FloorPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor005"));
    const FString NextPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor006"));
    const TCHAR* CheckpointOption = TEXT("AbyssCheckpoint");

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
            ? Italian : English;
    }

    bool CanUseFloor(const UEidolonCampaignSubsystem& Campaign)
    {
        return Campaign.CurrentChapter().Id == EidolonAbyss::Chapter && Campaign.GetEnding().IsNone()
            && Campaign.GetAuthoredAbyssCompletedFloor() >= EidolonAbyss::MarrowFloor - 1;
    }

    bool CanReach(const AEidolonCharacter& Player, const ATargetPoint* Target)
    {
        return IsValid(Target) && EidolonQuest::CanReachInteractionTarget(Player, *Target, RangeCm);
    }
}

UEidolonCampaignSubsystem* AEidolonMarrowRamGameMode::GetCampaign() const
{
    auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

void AEidolonMarrowRamGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    auto* World = GetWorld();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != TEXT("AbyssFloor005") ||
        FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
    {
        return;
    }

    using EidolonAbyss::FindTaggedActor;
    Arena = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_005_encounter"));
    RamSpawn = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_005_marrow_ram_01"));
    Latch = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_005_latch"));
    Exit = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_005_exit"));
    Beam = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_005_collision_beam"));
    Gate = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_005_ram_gate"));
    bBound = Arena.IsValid() && RamSpawn.IsValid() && Latch.IsValid() && Exit.IsValid()
        && Beam.IsValid() && Gate.IsValid()
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_005_player_start"))
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_005_checkpoint_start"));
    if (!bBound)
    {
        Notice = Display(TEXT("The portico layout is incomplete."), TEXT("La mappa del portico è incompleta."));
        return;
    }

    const auto* Campaign = GetCampaign();
    SetGateOpen(Campaign && CanUseFloor(*Campaign) && Campaign->GetAbyssMarrowLatchStage() == Stage::Open);
}

AActor* AEidolonMarrowRamGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* World = GetWorld())
    {
        const FName Tag = UGameplayStatics::HasOption(OptionsString, CheckpointOption)
            ? TEXT("abyss_005_checkpoint_start") : TEXT("abyss_005_player_start");
        if (auto* Start = EidolonAbyss::FindTaggedActor<APlayerStart>(*World, Tag)) return Start;
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

void AEidolonMarrowRamGameMode::Interact(AEidolonCharacter* Player)
{
    auto* Campaign = GetCampaign();
    if (!bBound || !Gate.IsValid() || !IsValid(Player) || Player->IsDefeated() ||
        Player != UGameplayStatics::GetPlayerCharacter(this, 0) || !Campaign || !CanUseFloor(*Campaign))
    {
        return;
    }

    const Stage Current = Campaign->GetAbyssMarrowLatchStage();
    if (Current == Stage::Open)
    {
        if (!CanReach(*Player, Exit.Get())) return;
        if (FPackageName::DoesPackageExist(NextPackage))
            UGameplayStatics::OpenLevel(this, FName(*NextPackage));
        else
            Notice = Display(TEXT("The route below is unavailable."), TEXT("Il passaggio al piano inferiore non è disponibile."));
        return;
    }

    const bool bAtLatch = CanReach(*Player, Latch.Get());
    const bool bAtArena = CanReach(*Player, Arena.Get());
    if (PendingRelease && (bAtArena || bAtLatch))
    {
        if (SaveLatch(*Campaign, *PendingRelease)) PendingRelease.reset();
        return;
    }

    if (EidolonMarrowLatch::IsReleased(Current) && bAtLatch)
    {
        if (SaveLatch(*Campaign, Signal::OperateLatch)) SetGateOpen(true);
        return;
    }
    if (bAtArena && Current == Stage::Locked && !IsValid(ActiveRam.Get()))
        SpawnRam();
}

void AEidolonMarrowRamGameMode::SpawnRam()
{
    const auto* Marker = RamSpawn.Get();
    if (!Marker) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    ActiveRam = GetWorld()->SpawnActor<AEidolonMarrowRam>(
        Marker->GetActorLocation() + FVector(0, 0, RamSpawnHeightCm), Marker->GetActorRotation(), Params);
    if (!IsValid(ActiveRam.Get()))
    {
        Notice = Display(TEXT("Clear the spawn area and interact to retry."),
                         TEXT("Libera il punto di comparsa e interagisci per riprovare."));
        return;
    }
    ActiveRam->OnChargeObstructed.AddUObject(this, &AEidolonMarrowRamGameMode::OnBeamImpact);
    ActiveRam->OnEncounterDefeated.AddUObject(this, &AEidolonMarrowRamGameMode::OnRamDefeated);
    Notice.Reset();
}

void AEidolonMarrowRamGameMode::OnBeamImpact(AEidolonMarrowRam& Ram, AActor& Obstacle)
{
    if (ActiveRam.Get() == &Ram && Beam.Get() == &Obstacle) RecordRelease(Signal::BeamImpact);
}

void AEidolonMarrowRamGameMode::OnRamDefeated(AEidolonArenaEnemy& Enemy)
{
    if (ActiveRam.Get() != &Enemy) return;
    ActiveRam = nullptr;
    RecordRelease(Signal::RamDefeat);
}

void AEidolonMarrowRamGameMode::RecordRelease(Signal Signal)
{
    auto* Campaign = GetCampaign();
    if (!bBound || !Campaign || Campaign->GetAbyssMarrowLatchStage() != Stage::Locked) return;
    if (!PendingRelease) PendingRelease = Signal;
    if (SaveLatch(*Campaign, *PendingRelease)) PendingRelease.reset();
}

bool AEidolonMarrowRamGameMode::SaveLatch(UEidolonCampaignSubsystem& Campaign, Signal Signal)
{
    if (!Campaign.CommitAbyssMarrowLatch(Signal, Campaign.GetAbyssMarrowLatchStage()))
    {
        Notice = Display(TEXT("Checkpoint failed. Interact to retry."),
                         TEXT("Salvataggio fallito. Interagisci per riprovare."));
        return false;
    }
    Notice.Reset();
    return true;
}

void AEidolonMarrowRamGameMode::SetGateOpen(bool bOpen)
{
    if (auto* Door = Gate.Get())
    {
        Door->SetActorEnableCollision(!bOpen);
        Door->SetActorHiddenInGame(bOpen);
    }
}

void AEidolonMarrowRamGameMode::ReloadCheckpoint()
{
    if (!bBound) return;
    if (auto* Campaign = GetCampaign())
    {
        if (!Campaign->LoadCampaign())
            UE_LOG(LogTemp, Warning, TEXT("Portico checkpoint could not be read. Preserving current state."));
        if (CanUseFloor(*Campaign)) UGameplayStatics::OpenLevel(this, FloorPackage, true, CheckpointOption);
    }
}

FString AEidolonMarrowRamGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    const auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign))
        return Display(TEXT("Enter after completing the previous floor."), TEXT("Entra dopo aver completato il piano precedente."));
    const Stage Current = Campaign->GetAbyssMarrowLatchStage();
    if (Current == Stage::Open)
        return Display(TEXT("Use the lower stair."), TEXT("Usa la scala verso il basso."));
    if (EidolonMarrowLatch::IsReleased(Current))
        return Display(TEXT("Operate the latch under the bridge."), TEXT("Aziona il fermo sotto la passerella."));
    if (IsValid(ActiveRam.Get()))
        return Display(TEXT("Bait the charge into the beam or defeat the Ram."),
                       TEXT("Attira la carica contro la traversa o sconfiggi l'Ariete."));
    return Display(TEXT("Interact at the encounter marker."), TEXT("Interagisci nel punto dell'incontro."));
}

FString AEidolonMarrowRamGameMode::GetNarrative() const
{
    return Display(TEXT("Broken Portico | Abyss, floor 5"), TEXT("Portico spezzato | Abisso, livello 5"));
}
