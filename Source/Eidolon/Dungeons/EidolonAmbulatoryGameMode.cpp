#include "EidolonAmbulatoryGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonAbyssProgression.h"
#include "EidolonRattleMillipede.h"
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
    using namespace EidolonCounterweight;
    constexpr double RangeCm = 180.0;
    constexpr double FloorLossHeightCm = -150.0;
    constexpr double PatrolHeightCm = 42.0;
    const FName FloorPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor006"));
    const FString NextPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor007"));
    const TCHAR* CheckpointOption = TEXT("AbyssCheckpoint");

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
            ? Italian : English;
    }

    bool CanUseFloor(const UEidolonCampaignSubsystem& Campaign)
    {
        return Campaign.CurrentChapter().Id == EidolonAbyss::Chapter && Campaign.GetEnding().IsNone()
            && Campaign.GetAuthoredAbyssCompletedFloor() >= EidolonAbyss::CounterweightFloor - 1;
    }

    bool CanReach(const AEidolonCharacter& Player, const ATargetPoint* Target)
    {
        return IsValid(Target) && EidolonQuest::CanReachInteractionTarget(Player, *Target, RangeCm);
    }
}

AEidolonAmbulatoryGameMode::AEidolonAmbulatoryGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
}

UEidolonCampaignSubsystem* AEidolonAmbulatoryGameMode::GetCampaign() const
{
    auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

void AEidolonAmbulatoryGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    auto* World = GetWorld();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != TEXT("AbyssFloor006") ||
        FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
    {
        return;
    }

    using EidolonAbyss::FindTaggedActor;
    Sarcophagus = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_006_sarcophagus"));
    Winch = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_006_winch"));
    WeightSeat = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_006_counterweight_seat"));
    Exit = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_006_exit"));
    Counterweight = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_006_counterweight"));
    Gate = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_006_ambulatory_gate"));
    bBound = Sarcophagus.IsValid() && Winch.IsValid() && WeightSeat.IsValid() && Exit.IsValid() &&
        Counterweight.IsValid() && Gate.IsValid()
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_006_player_start"))
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_006_checkpoint_start"));
    if (!bBound)
    {
        Notice = Display(TEXT("The ambulatory layout is incomplete."), TEXT("La mappa dell'ambulacro è incompleta."));
        return;
    }

    SourceTransform = Counterweight->GetActorTransform();
    const auto* Campaign = GetCampaign();
    const bool bCanUseFloor = Campaign && CanUseFloor(*Campaign);
    const Progress Current = bCanUseFloor ? Campaign->GetAbyssCounterweightProgress() : Progress{};
    UpdateWeight(Current);
    if (bCanUseFloor && !CanExit(Current)) SpawnPatrol();
}

void AEidolonAmbulatoryGameMode::SpawnPatrol()
{
    auto* World = GetWorld();
    TArray<FVector> Patrol;
    Patrol.Reserve(4);
    for (int32 Index = 1; Index <= 4; ++Index)
    {
        const FName Tag(*FString::Printf(TEXT("abyss_006_patrol_%02d"), Index));
        const auto* Anchor = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, Tag);
        if (!Anchor)
        {
            UE_LOG(LogTemp, Warning, TEXT("The optional millipede patrol has incomplete anchors."));
            return;
        }
        Patrol.Add(Anchor->GetActorLocation() + FVector(0, 0, PatrolHeightCm));
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    if (auto* Enemy = World->SpawnActor<AEidolonRattleMillipede>(
            Patrol[0], (Patrol[1] - Patrol[0]).Rotation(), Params))
    {
        if (!Enemy->SetPatrolRoute(Patrol)) Enemy->Destroy();
    }
}

AActor* AEidolonAmbulatoryGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* World = GetWorld())
    {
        const FName Tag = UGameplayStatics::HasOption(OptionsString, CheckpointOption)
            ? TEXT("abyss_006_checkpoint_start") : TEXT("abyss_006_player_start");
        if (auto* Start = EidolonAbyss::FindTaggedActor<APlayerStart>(*World, Tag)) return Start;
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

void AEidolonAmbulatoryGameMode::Tick(float DeltaSeconds)
{
    AGameModeBase::Tick(DeltaSeconds);
    if (!bBound || bRecoveryAttempted) return;
    auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign)) return;

    const Progress Current = Campaign->GetAbyssCounterweightProgress();
    if (Current.Weight != Location::Carried) return;
    const auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Player || Player->IsDefeated() || Player->GetActorLocation().Z < FloorLossHeightCm)
    {
        bRecoveryAttempted = true;
        ApplyEvent(*Campaign, {Current, Action::Recover, Evidence::CarrierRecovery});
    }
}

void AEidolonAmbulatoryGameMode::Interact(AEidolonCharacter* Player)
{
    auto* Campaign = GetCampaign();
    if (!bBound || !Gate.IsValid() || !Counterweight.IsValid() || !IsValid(Player) || Player->IsDefeated() ||
        Player != UGameplayStatics::GetPlayerCharacter(this, 0) || !Campaign || !CanUseFloor(*Campaign))
    {
        return;
    }

    const Progress Current = Campaign->GetAbyssCounterweightProgress();
    if (CanExit(Current))
    {
        if (!CanReach(*Player, Exit.Get())) return;
        if (FPackageName::DoesPackageExist(NextPackage))
            UGameplayStatics::OpenLevel(this, FName(*NextPackage));
        else
            Notice = Display(TEXT("The route below is unavailable."), TEXT("Il passaggio al piano inferiore non è disponibile."));
        return;
    }

    if (Current.Weight == Location::AtSource && CanReach(*Player, Sarcophagus.Get()))
        ApplyEvent(*Campaign, {Current, Action::Collect, Evidence::Source});
    else if (Current.Weight == Location::Carried && CanReach(*Player, Winch.Get()))
        ApplyEvent(*Campaign, {Current, Action::Install, Evidence::Winch});
}

bool AEidolonAmbulatoryGameMode::ApplyEvent(UEidolonCampaignSubsystem& Campaign, const Event& Event)
{
    if (!Campaign.CommitAbyssCounterweight(Event))
    {
        Notice = Display(TEXT("Checkpoint failed. Try again."),
                         TEXT("Salvataggio fallito. Riprova."));
        return false;
    }

    bRecoveryAttempted = false;
    Notice.Reset();
    UpdateWeight(Campaign.GetAbyssCounterweightProgress());
    return true;
}

void AEidolonAmbulatoryGameMode::UpdateWeight(Progress Progress)
{
    const bool bInstalled = CanExit(Progress);
    if (auto* Door = Gate.Get())
    {
        Door->SetActorEnableCollision(!bInstalled);
        Door->SetActorHiddenInGame(bInstalled);
    }

    auto* Weight = Counterweight.Get();
    if (!Weight) return;
    Weight->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Weight->SetActorEnableCollision(false);
    Weight->SetActorTransform(SourceTransform);

    if (bInstalled)
    {
        if (const auto* Target = WeightSeat.Get())
            Weight->SetActorLocation(Target->GetActorLocation());
    }
    else if (Progress.Weight == Location::Carried)
    {
        if (auto* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
        {
            Weight->AttachToActor(Player, FAttachmentTransformRules::KeepWorldTransform);
            Weight->SetActorRelativeLocation(FVector(70, 0, 30));
        }
    }
}

void AEidolonAmbulatoryGameMode::ReloadCheckpoint()
{
    if (!bBound) return;
    if (auto* Campaign = GetCampaign(); Campaign && CanUseFloor(*Campaign))
    {
        const Progress Current = Campaign->GetAbyssCounterweightProgress();
        if (Current.Weight == Location::Carried &&
            !ApplyEvent(*Campaign, {Current, Action::Recover, Evidence::CarrierRecovery}))
        {
            return;
        }

        if (!Campaign->LoadCampaign())
            UE_LOG(LogTemp, Warning, TEXT("Ambulatory checkpoint could not be read. Preserving current state."));
        if (CanUseFloor(*Campaign)) UGameplayStatics::OpenLevel(this, FloorPackage, true, CheckpointOption);
    }
}

FString AEidolonAmbulatoryGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    const auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign))
        return Display(TEXT("Enter after completing the previous floor."), TEXT("Entra dopo aver completato il piano precedente."));

    switch (Campaign->GetAbyssCounterweightProgress().Weight)
    {
        case Location::AtSource:
            return Display(TEXT("Collect the counterweight from the sarcophagus."),
                           TEXT("Prendi il contrappeso dal sarcofago."));
        case Location::Carried:
            return Display(TEXT("Carry the weight to the winch."), TEXT("Porta il contrappeso all'argano."));
        case Location::Installed:
            return Display(TEXT("The gate is open. Use the lower stair."), TEXT("Il varco è aperto. Usa la scala verso il basso."));
    }
    return {};
}

FString AEidolonAmbulatoryGameMode::GetNarrative() const
{
    return Display(TEXT("Double-Ring Ambulatory | Abyss, floor 6"), TEXT("Ambulacro a doppio anello | Abisso, livello 6"));
}
