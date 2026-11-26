#include "EidolonSluiceGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonAbyssProgression.h"
#include "EidolonSluiceRules.h"
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
    constexpr double RangeCm = 180.0;
    const FName FloorPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor004"));
    const FString NextPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor005"));
    const TCHAR* CheckpointOption = TEXT("AbyssCheckpoint");

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
            ? Italian : English;
    }

    bool CanUseFloor(const UEidolonCampaignSubsystem& Campaign)
    {
        return Campaign.CurrentChapter().Id == EidolonAbyss::Chapter && Campaign.GetEnding().IsNone()
            && Campaign.GetAuthoredAbyssCompletedFloor() >= EidolonAbyss::SluiceFloor - 1;
    }

    bool CanReach(const AEidolonCharacter& Player, const ATargetPoint* Target)
    {
        return IsValid(Target) && EidolonQuest::CanReachInteractionTarget(Player, *Target, RangeCm);
    }
}

UEidolonCampaignSubsystem* AEidolonSluiceGameMode::GetCampaign() const
{
    auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

void AEidolonSluiceGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    auto* World = GetWorld();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != TEXT("AbyssFloor004") ||
        FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
    {
        return;
    }

    West = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_004_sluice_west"));
    East = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_004_sluice_east"));
    Exit = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_004_exit"));
    Gate = EidolonAbyss::FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_004_sluice_gate"));
    Water = EidolonAbyss::FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_004_water"));
    bBound = West.IsValid() && East.IsValid() && Exit.IsValid() && Gate.IsValid() && Water.IsValid()
        && EidolonAbyss::FindTaggedActor<APlayerStart>(*World, TEXT("abyss_004_player_start"))
        && EidolonAbyss::FindTaggedActor<APlayerStart>(*World, TEXT("abyss_004_checkpoint_start"));
    if (!bBound)
    {
        Notice = Display(TEXT("The canals layout is incomplete."), TEXT("La mappa dei canali è incompleta."));
        return;
    }

    InitialWaterPosition = Water->GetActorLocation();
    const auto* Campaign = GetCampaign();
    UpdateSluices(Campaign && CanUseFloor(*Campaign) ? Campaign->GetAbyssSluiceProgress() : 0);
}

AActor* AEidolonSluiceGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* World = GetWorld())
    {
        const FName Tag = UGameplayStatics::HasOption(OptionsString, CheckpointOption)
            ? TEXT("abyss_004_checkpoint_start") : TEXT("abyss_004_player_start");
        if (auto* Start = EidolonAbyss::FindTaggedActor<APlayerStart>(*World, Tag)) return Start;
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

void AEidolonSluiceGameMode::Interact(AEidolonCharacter* Player)
{
    using namespace EidolonSluice;
    auto* Campaign = GetCampaign();
    if (!bBound || !Gate.IsValid() || !IsValid(Player) || Player->IsDefeated() ||
        Player != UGameplayStatics::GetPlayerCharacter(this, 0) || !Campaign || !CanUseFloor(*Campaign))
    {
        return;
    }

    const Progress Current = Campaign->GetAbyssSluiceProgress();
    if (CanExit(Current))
    {
        if (!CanReach(*Player, Exit.Get())) return;
        if (FPackageName::DoesPackageExist(NextPackage))
            UGameplayStatics::OpenLevel(this, FName(*NextPackage));
        else
            Notice = Display(TEXT("The route below is unavailable."), TEXT("Il passaggio al piano inferiore non è disponibile."));
        return;
    }

    const bool bWest = CanReach(*Player, West.Get());
    if (!bWest && !CanReach(*Player, East.Get())) return;
    const Control Lever = bWest ? Control::Inlet : Control::Outlet;
    if (IsActivated(Current, Lever))
    {
        Notice = Display(TEXT("This sluice is open. Find the other one."),
                         TEXT("Questa chiusa è aperta. Cerca l'altra."));
        return;
    }
    if (!Campaign->CommitAbyssSluice(Lever, Current))
    {
        Notice = Display(TEXT("Checkpoint failed. Interact to retry."),
                         TEXT("Salvataggio fallito. Interagisci per riprovare."));
        return;
    }
    Notice.Reset();
    UpdateSluices(Campaign->GetAbyssSluiceProgress());
}

void AEidolonSluiceGameMode::UpdateSluices(uint8 Progress)
{
    const bool bDrained = EidolonSluice::CanExit(Progress);
    if (auto* Door = Gate.Get())
    {
        Door->SetActorEnableCollision(!bDrained);
        Door->SetActorHiddenInGame(bDrained);
    }
    if (auto* Basin = Water.Get())
    {
        Basin->SetActorLocation(Progress == 0 ? InitialWaterPosition
            : FVector(InitialWaterPosition.X, InitialWaterPosition.Y, 0.0));
        Basin->SetActorHiddenInGame(bDrained);
    }
}

void AEidolonSluiceGameMode::ReloadCheckpoint()
{
    if (!bBound) return;
    if (auto* Campaign = GetCampaign())
    {
        if (!Campaign->LoadCampaign())
            UE_LOG(LogTemp, Warning, TEXT("Sluice checkpoint could not be read. Preserving current state."));
        if (CanUseFloor(*Campaign)) UGameplayStatics::OpenLevel(this, FloorPackage, true, CheckpointOption);
    }
}

FString AEidolonSluiceGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    const auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign))
        return Display(TEXT("Enter after completing the previous floor."), TEXT("Entra dopo aver completato il piano precedente."));
    const auto Progress = Campaign->GetAbyssSluiceProgress();
    if (EidolonSluice::CanExit(Progress))
        return Display(TEXT("The ramp is clear. Use the lower stair."), TEXT("La rampa è libera. Usa la scala verso il basso."));
    return Progress == 0
        ? Display(TEXT("Open both sluices."), TEXT("Apri entrambe le chiuse."))
        : Display(TEXT("Open the remaining sluice."), TEXT("Apri la chiusa rimanente."));
}

FString AEidolonSluiceGameMode::GetNarrative() const
{
    return Display(TEXT("Drainage Canals | Abyss, floor 4"), TEXT("Canali di scolo | Abisso, livello 4"));
}
