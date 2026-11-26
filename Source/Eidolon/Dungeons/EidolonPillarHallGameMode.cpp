#include "EidolonPillarHallGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonAbyssProgression.h"
#include "EidolonMortarLeech.h"
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
    constexpr double LeechHeightCm = 45.0;
    constexpr double PointerDistanceCm = 85.0;
    const FName FloorPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor007"));
    const FString NextPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor008"));
    const TCHAR* CheckpointOption = TEXT("AbyssCheckpoint");

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
            ? Italian : English;
    }

    bool CanUseFloor(const UEidolonCampaignSubsystem& Campaign)
    {
        return Campaign.CurrentChapter().Id == EidolonAbyss::Chapter && Campaign.GetEnding().IsNone()
            && Campaign.GetAuthoredAbyssCompletedFloor() >= EidolonAbyss::MirrorFloor - 1;
    }

    bool CanReach(const AEidolonCharacter& Player, const ATargetPoint* Target)
    {
        return IsValid(Target) && EidolonQuest::CanReachInteractionTarget(Player, *Target, RangeCm);
    }
}

UEidolonCampaignSubsystem* AEidolonPillarHallGameMode::GetCampaign() const
{
    auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

void AEidolonPillarHallGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    bBound = BindMirrors();
    if (!bBound)
    {
        Notice = Display(TEXT("The pillar hall layout is incomplete."), TEXT("La mappa della sala dei pilastri è incompleta."));
        return;
    }

    const auto* Campaign = GetCampaign();
    const bool bCanUseFloor = Campaign && CanUseFloor(*Campaign);
    const auto Progress = bCanUseFloor ? Campaign->GetAbyssMirrorProgress() : EidolonMirror::Progress{};
    UpdateMirrors(Progress);
    if (bCanUseFloor && !EidolonMirror::CanExit(Progress)) SpawnLeeches();
}

bool AEidolonPillarHallGameMode::BindMirrors()
{
    auto* World = GetWorld();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != TEXT("AbyssFloor007") ||
        FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
    {
        return false;
    }

    using EidolonAbyss::FindTaggedActor;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FString Tag = FString::Printf(TEXT("abyss_007_mirror_%02d"), Index + 1);
        auto& Mirror = Mirrors[Index];
        Mirror.Control = FindTaggedActor<ATargetPoint>(*World, FName(*Tag));
        Mirror.Panel = FindTaggedActor<AStaticMeshActor>(*World, FName(*(Tag + TEXT("_mesh"))));
        Mirror.Pointer = FindTaggedActor<AStaticMeshActor>(*World, FName(*(Tag + TEXT("_pointer"))));
        if (!Mirror.Control.IsValid() || !Mirror.Panel.IsValid() || !Mirror.Pointer.IsValid()) return false;
    }

    Exit = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_007_exit"));
    Gate = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_007_pillar_gate"));
    return Exit.IsValid() && Gate.IsValid()
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_007_player_start"))
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_007_checkpoint_start"));
}

void AEidolonPillarHallGameMode::SpawnLeeches()
{
    auto* World = GetWorld();
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    for (int32 Index = 1; Index <= 2; ++Index)
    {
        const FName Tag(*FString::Printf(TEXT("abyss_007_mortar_leech_%02d"), Index));
        const auto* Anchor = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, Tag);
        if (Anchor && World->SpawnActor<AEidolonMortarLeech>(
            Anchor->GetActorLocation() + FVector(0, 0, LeechHeightCm), Anchor->GetActorRotation(), Params))
        {
            continue;
        }
        UE_LOG(LogTemp, Warning, TEXT("The optional Leech could not spawn at %s."), *Tag.ToString());
    }
}

AActor* AEidolonPillarHallGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* World = GetWorld())
    {
        const FName Tag = UGameplayStatics::HasOption(OptionsString, CheckpointOption)
            ? TEXT("abyss_007_checkpoint_start") : TEXT("abyss_007_player_start");
        if (auto* Start = EidolonAbyss::FindTaggedActor<APlayerStart>(*World, Tag)) return Start;
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

void AEidolonPillarHallGameMode::Interact(AEidolonCharacter* Player)
{
    auto* Campaign = GetCampaign();
    if (!bBound || !Gate.IsValid() || !IsValid(Player) || Player->IsDefeated() ||
        Player != UGameplayStatics::GetPlayerCharacter(this, 0) || !Campaign || !CanUseFloor(*Campaign))
    {
        return;
    }

    const auto Current = Campaign->GetAbyssMirrorProgress();
    if (EidolonMirror::CanExit(Current))
    {
        if (!CanReach(*Player, Exit.Get())) return;
        if (FPackageName::DoesPackageExist(NextPackage))
            UGameplayStatics::OpenLevel(this, FName(*NextPackage));
        else
            Notice = Display(TEXT("The route below is unavailable."), TEXT("Il passaggio al piano inferiore non è disponibile."));
        return;
    }

    for (int32 Index = 0; Index < 2; ++Index)
    {
        if (!CanReach(*Player, Mirrors[Index].Control.Get())) continue;
        const auto Control = Index == 0 ? EidolonMirror::Control::First : EidolonMirror::Control::Second;
        if (!Campaign->CommitAbyssMirror({Current, Control}))
        {
            Notice = Display(TEXT("Checkpoint failed. Rotate again to retry."),
                             TEXT("Salvataggio fallito. Ruota di nuovo per riprovare."));
            return;
        }
        Notice.Reset();
        UpdateMirrors(Campaign->GetAbyssMirrorProgress());
        return;
    }
}

void AEidolonPillarHallGameMode::UpdateMirrors(const EidolonMirror::Progress& Progress)
{
    constexpr double QuarterTurnDegrees = 90.0;
    const uint8 Orientations[] = {Progress.First, Progress.Second};
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const auto& Mirror = Mirrors[Index];
        auto* Panel = Mirror.Panel.Get();
        auto* Pointer = Mirror.Pointer.Get();
        if (!Panel || !Pointer) continue;
        const FRotator Rotation(0, QuarterTurnDegrees * Orientations[Index], 0);
        Panel->SetActorRotation(Rotation);
        Pointer->SetActorLocationAndRotation(Panel->GetActorLocation() + Rotation.Vector() * PointerDistanceCm, Rotation);
    }
    if (auto* Door = Gate.Get())
    {
        const bool bAligned = EidolonMirror::CanExit(Progress);
        Door->SetActorEnableCollision(!bAligned);
        Door->SetActorHiddenInGame(bAligned);
    }
}

void AEidolonPillarHallGameMode::ReloadCheckpoint()
{
    if (!bBound) return;
    if (auto* Campaign = GetCampaign())
    {
        if (!Campaign->LoadCampaign())
            UE_LOG(LogTemp, Warning, TEXT("Pillar hall checkpoint could not be read. Preserving current state."));
        if (CanUseFloor(*Campaign)) UGameplayStatics::OpenLevel(this, FloorPackage, true, CheckpointOption);
    }
}

FString AEidolonPillarHallGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    const auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign))
        return Display(TEXT("Enter after completing the previous floor."), TEXT("Entra dopo aver completato il piano precedente."));
    if (EidolonMirror::CanExit(Campaign->GetAbyssMirrorProgress()))
        return Display(TEXT("The path is open. Use the lower stair."), TEXT("Il varco è aperto. Usa la scala verso il basso."));
    return Display(TEXT("Turn both mirrors toward their receivers."),
                   TEXT("Orienta entrambi gli specchi verso i ricevitori."));
}

FString AEidolonPillarHallGameMode::GetNarrative() const
{
    return Display(TEXT("Pillar Hall | Abyss, floor 7"), TEXT("Sala dei pilastri | Abisso, livello 7"));
}
