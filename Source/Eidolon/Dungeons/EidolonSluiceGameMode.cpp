#include "EidolonSluiceGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonAbyssProgression.h"
#include "EidolonCenserHagfish.h"
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
    constexpr double HagfishHeightCm = 50.0;
    const TCHAR* MapFolder = TEXT("/Game/Eidolon/Maps");
    const TCHAR* CheckpointOption = TEXT("AbyssCheckpoint");

    FString FloorName(int32 Floor)
    {
        return FString::Printf(TEXT("AbyssFloor%03d"), Floor);
    }

    FString FloorPackage(int32 Floor)
    {
        return FString(MapFolder) / FloorName(Floor);
    }

    FName FloorTag(int32 Floor, const TCHAR* Suffix)
    {
        return FName(*FString::Printf(TEXT("abyss_%03d_%s"), Floor, Suffix));
    }

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
            ? Italian : English;
    }

    bool CanReach(const AEidolonCharacter& Player, const ATargetPoint* Target)
    {
        return IsValid(Target) && EidolonQuest::CanReachInteractionTarget(Player, *Target, RangeCm);
    }
}

FEidolonDrainLayout AEidolonSluiceGameMode::GetDrainLayout() const
{
    return {EidolonAbyss::SluiceFloor, TEXT("abyss_004_sluice_west"), TEXT("abyss_004_sluice_east"),
        TEXT("abyss_004_sluice_gate"), 0.0, 0.0, true,
        TEXT("Drainage Canals | Abyss, floor 4"), TEXT("Canali di scolo | Abisso, livello 4"),
        TEXT("Open both sluices."), TEXT("Apri entrambe le chiuse."),
        TEXT("Open the remaining sluice."), TEXT("Apri la chiusa rimanente."),
        TEXT("The ramp is clear. Use the lower stair."), TEXT("La rampa è libera. Usa la scala verso il basso."),
        TEXT("This sluice is open. Find the other one."), TEXT("Questa chiusa è aperta. Cerca l'altra.")};
}

AActor* AEidolonSluiceGameMode::SpawnThreat(UWorld& World)
{
    const auto* Anchor = EidolonAbyss::FindTaggedActor<ATargetPoint>(World, TEXT("abyss_004_censer_hagfish_01"));
    if (!Anchor) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    return World.SpawnActor<AEidolonCenserHagfish>(Anchor->GetActorLocation() + FVector(0, 0, HagfishHeightCm),
        Anchor->GetActorRotation(), Params);
}

UEidolonCampaignSubsystem* AEidolonSluiceGameMode::GetCampaign() const
{
    auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

bool AEidolonSluiceGameMode::CanUseFloor(const UEidolonCampaignSubsystem& Campaign) const
{
    return Campaign.CurrentChapter().Id == EidolonAbyss::Chapter && Campaign.GetEnding().IsNone()
        && Campaign.GetAuthoredAbyssCompletedFloor() >= GetDrainLayout().Floor - 1;
}

void AEidolonSluiceGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    auto* World = GetWorld();
    const auto Layout = GetDrainLayout();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != FloorName(Layout.Floor) ||
        FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != MapFolder)
    {
        return;
    }

    using EidolonAbyss::FindTaggedActor;
    First = FindTaggedActor<ATargetPoint>(*World, Layout.FirstControl);
    Second = FindTaggedActor<ATargetPoint>(*World, Layout.SecondControl);
    Exit = FindTaggedActor<ATargetPoint>(*World, FloorTag(Layout.Floor, TEXT("exit")));
    Gate = FindTaggedActor<AStaticMeshActor>(*World, Layout.Gate);
    Water = FindTaggedActor<AStaticMeshActor>(*World, FloorTag(Layout.Floor, TEXT("water")));
    bBound = First.IsValid() && Second.IsValid() && Exit.IsValid() && Gate.IsValid() && Water.IsValid()
        && FindTaggedActor<APlayerStart>(*World, FloorTag(Layout.Floor, TEXT("player_start")))
        && FindTaggedActor<APlayerStart>(*World, FloorTag(Layout.Floor, TEXT("checkpoint_start")));
    if (!bBound)
    {
        Notice = Layout.Floor == EidolonAbyss::SluiceFloor
            ? Display(TEXT("The canals layout is incomplete."), TEXT("La mappa dei canali è incompleta."))
            : Display(TEXT("This Abyss floor is incomplete."), TEXT("Questo piano dell'Abisso è incompleto."));
        return;
    }

    InitialWaterPosition = Water->GetActorLocation();
    const auto* Campaign = GetCampaign();
    const bool bCanUseFloor = Campaign && CanUseFloor(*Campaign);
    const uint8 Progress = bCanUseFloor ? Campaign->GetAbyssDrainProgress(Layout.Floor) : 0;
    UpdateSluices(Progress);

    if (bCanUseFloor && (!Layout.bRemoveThreatWhenDrained || !EidolonSluice::CanExit(Progress)))
    {
        Threat = SpawnThreat(*World);
        if (!Threat.IsValid())
            UE_LOG(LogTemp, Warning, TEXT("The optional threat could not be spawned in %s."), *FloorName(Layout.Floor));
    }
}

AActor* AEidolonSluiceGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* World = GetWorld())
    {
        const FName Tag = FloorTag(GetDrainLayout().Floor, UGameplayStatics::HasOption(OptionsString, CheckpointOption)
            ? TEXT("checkpoint_start") : TEXT("player_start"));
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

    const auto Layout = GetDrainLayout();
    const Progress Current = Campaign->GetAbyssDrainProgress(Layout.Floor);
    if (CanExit(Current))
    {
        if (!CanReach(*Player, Exit.Get())) return;
        const FString Destination = FloorPackage(Layout.Floor + 1);
        if (FPackageName::DoesPackageExist(Destination))
            UGameplayStatics::OpenLevel(this, FName(*Destination));
        else
            Notice = Display(TEXT("The route below is unavailable."), TEXT("Il passaggio al piano inferiore non è disponibile."));
        return;
    }

    const bool bFirst = CanReach(*Player, First.Get());
    if (!bFirst && !CanReach(*Player, Second.Get())) return;
    const Control Lever = bFirst ? Control::Inlet : Control::Outlet;
    if (IsActivated(Current, Lever))
    {
        Notice = Display(Layout.DuplicateEnglish, Layout.DuplicateItalian);
        return;
    }
    if (!Campaign->CommitAbyssDrain(Layout.Floor, Lever, Current))
    {
        Notice = Display(TEXT("Checkpoint failed. Interact to retry."),
                         TEXT("Salvataggio fallito. Interagisci per riprovare."));
        return;
    }
    Notice.Reset();
    UpdateSluices(Campaign->GetAbyssDrainProgress(Layout.Floor));
}

void AEidolonSluiceGameMode::UpdateSluices(uint8 Progress)
{
    const bool bDrained = EidolonSluice::CanExit(Progress);
    const auto Layout = GetDrainLayout();
    if (auto* Door = Gate.Get())
    {
        Door->SetActorEnableCollision(!bDrained);
        Door->SetActorHiddenInGame(bDrained);
    }
    if (auto* Basin = Water.Get())
    {
        FVector WaterPosition = InitialWaterPosition;
        if (Progress != 0) WaterPosition.Z = bDrained ? Layout.EmptyWaterZ : Layout.PartialWaterZ;
        Basin->SetActorLocation(WaterPosition);
        Basin->SetActorHiddenInGame(bDrained);
    }
    if (bDrained && Layout.bRemoveThreatWhenDrained)
    {
        if (auto* Enemy = Threat.Get()) Enemy->Destroy();
    }
}

void AEidolonSluiceGameMode::ReloadCheckpoint()
{
    if (!bBound) return;
    if (auto* Campaign = GetCampaign())
    {
        if (!Campaign->LoadCampaign())
            UE_LOG(LogTemp, Warning, TEXT("Sluice checkpoint could not be read. Preserving current state."));
        if (CanUseFloor(*Campaign))
            UGameplayStatics::OpenLevel(this, FName(*FloorPackage(GetDrainLayout().Floor)), true, CheckpointOption);
    }
}

FString AEidolonSluiceGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    const auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign))
        return Display(TEXT("Enter after completing the previous floor."), TEXT("Entra dopo aver completato il piano precedente."));
    const auto Layout = GetDrainLayout();
    const auto Progress = Campaign->GetAbyssDrainProgress(Layout.Floor);
    if (EidolonSluice::CanExit(Progress))
        return Display(Layout.OpenEnglish, Layout.OpenItalian);
    return Progress == 0
        ? Display(Layout.BothEnglish, Layout.BothItalian)
        : Display(Layout.RemainingEnglish, Layout.RemainingItalian);
}

FString AEidolonSluiceGameMode::GetNarrative() const
{
    const auto Layout = GetDrainLayout();
    return Display(Layout.TitleEnglish, Layout.TitleItalian);
}
