#include "EidolonUrnForkGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonAbyssProgression.h"
#include "EidolonCenserHagfish.h"
#include "EidolonLintelMantis.h"
#include "EidolonOssuaryTick.h"
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
    using EidolonBranchLatch::Route;

    constexpr double RangeCm = 180.0;
    const FName FloorPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor009"));
    const FString NextPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor010"));
    const TCHAR* CheckpointOption = TEXT("AbyssCheckpoint");

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
            ? Italian : English;
    }

    bool CanUseFloor(const UEidolonCampaignSubsystem& Campaign)
    {
        return Campaign.CurrentChapter().Id == EidolonAbyss::Chapter && Campaign.GetEnding().IsNone()
            && Campaign.GetAuthoredAbyssCompletedFloor() >= EidolonAbyss::BranchLatchFloor - 1;
    }

    bool CanReach(const AEidolonCharacter& Player, const ATargetPoint* Target)
    {
        return IsValid(Target) && EidolonQuest::CanReachInteractionTarget(Player, *Target, RangeCm);
    }

    template <typename EnemyType>
    void SpawnThreat(UWorld& World, FName Tag, double Height)
    {
        if (const auto* Anchor = EidolonAbyss::FindTaggedActor<ATargetPoint>(World, Tag))
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
            if (World.SpawnActor<EnemyType>(
                    Anchor->GetActorLocation() + FVector(0, 0, Height), Anchor->GetActorRotation(), Params))
            {
                return;
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("The optional threat could not spawn at %s."), *Tag.ToString());
    }
}

UEidolonCampaignSubsystem* AEidolonUrnForkGameMode::GetCampaign() const
{
    auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

void AEidolonUrnForkGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    bBound = BindFork();
    if (!bBound)
    {
        Notice = Display(TEXT("The urn fork layout is incomplete."), TEXT("La mappa del bivio è incompleta."));
        return;
    }

    const auto* Campaign = GetCampaign();
    const bool bCanUseFloor = Campaign && CanUseFloor(*Campaign);
    const Route Current = bCanUseFloor ? Campaign->GetAbyssBranchLatchRoute() : Route::Unselected;
    UpdateGate(Current);
    if (bCanUseFloor && !EidolonBranchLatch::CanExit(Current)) SpawnThreats();
}

bool AEidolonUrnForkGameMode::BindFork()
{
    auto* World = GetWorld();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != TEXT("AbyssFloor009") ||
        FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
    {
        return false;
    }

    using EidolonAbyss::FindTaggedActor;
    UpperLatch = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_009_latch_upper"));
    LowerLatch = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_009_latch_lower"));
    Exit = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_009_exit"));
    Gate = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_009_urn_gate"));
    return UpperLatch.IsValid() && LowerLatch.IsValid() && Exit.IsValid() && Gate.IsValid()
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_009_player_start"))
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_009_checkpoint_start"));
}

void AEidolonUrnForkGameMode::SpawnThreats()
{
    auto& World = *GetWorld();
    SpawnThreat<AEidolonLintelMantis>(World, TEXT("abyss_009_lintel_mantis_01"), 60.0);
    SpawnThreat<AEidolonOssuaryTick>(World, TEXT("abyss_009_ossuary_tick_01"), 30.0);
    SpawnThreat<AEidolonCenserHagfish>(World, TEXT("abyss_009_censer_hagfish_01"), 50.0);
}

AActor* AEidolonUrnForkGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* World = GetWorld())
    {
        const FName Tag = UGameplayStatics::HasOption(OptionsString, CheckpointOption)
            ? TEXT("abyss_009_checkpoint_start") : TEXT("abyss_009_player_start");
        if (auto* Start = EidolonAbyss::FindTaggedActor<APlayerStart>(*World, Tag)) return Start;
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

void AEidolonUrnForkGameMode::Interact(AEidolonCharacter* Player)
{
    auto* Campaign = GetCampaign();
    if (!bBound || !Gate.IsValid() || !IsValid(Player) || Player->IsDefeated() ||
        Player != UGameplayStatics::GetPlayerCharacter(this, 0) || !Campaign || !CanUseFloor(*Campaign))
    {
        return;
    }

    const Route Current = Campaign->GetAbyssBranchLatchRoute();
    if (EidolonBranchLatch::CanExit(Current))
    {
        if (!CanReach(*Player, Exit.Get())) return;
        if (FPackageName::DoesPackageExist(NextPackage))
            UGameplayStatics::OpenLevel(this, FName(*NextPackage));
        else
            Notice = Display(TEXT("The route below is unavailable."), TEXT("Il passaggio al piano inferiore non è disponibile."));
        return;
    }

    const bool bAtUpperLatch = CanReach(*Player, UpperLatch.Get());
    if (!bAtUpperLatch && !CanReach(*Player, LowerLatch.Get())) return;

    const Route Selected = bAtUpperLatch ? Route::First : Route::Second;
    if (!Campaign->CommitAbyssBranchLatch(Selected, Current))
    {
        Notice = Display(TEXT("Checkpoint failed. Interact to retry."),
                         TEXT("Salvataggio fallito. Interagisci per riprovare."));
        return;
    }

    Notice.Reset();
    UpdateGate(Campaign->GetAbyssBranchLatchRoute());
}

void AEidolonUrnForkGameMode::UpdateGate(Route Current)
{
    if (auto* Door = Gate.Get())
    {
        const bool bOpen = EidolonBranchLatch::CanExit(Current);
        Door->SetActorEnableCollision(!bOpen);
        Door->SetActorHiddenInGame(bOpen);
    }
}

void AEidolonUrnForkGameMode::ReloadCheckpoint()
{
    if (!bBound) return;
    if (auto* Campaign = GetCampaign())
    {
        if (!Campaign->LoadCampaign())
            UE_LOG(LogTemp, Warning, TEXT("Urn fork checkpoint could not be read. Preserving current state."));
        if (CanUseFloor(*Campaign)) UGameplayStatics::OpenLevel(this, FloorPackage, true, CheckpointOption);
    }
}

FString AEidolonUrnForkGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    const auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign))
        return Display(TEXT("Enter after completing the previous floor."), TEXT("Entra dopo aver completato il piano precedente."));
    if (EidolonBranchLatch::CanExit(Campaign->GetAbyssBranchLatchRoute()))
        return Display(TEXT("The gate is open. Use the lower stair."), TEXT("Il varco è aperto. Usa la scala verso il basso."));
    return Display(TEXT("Follow either passage and release its latch."),
                   TEXT("Scegli un passaggio e solleva il suo chiavistello."));
}

FString AEidolonUrnForkGameMode::GetNarrative() const
{
    return Display(TEXT("Urn Fork | Abyss, floor 9"), TEXT("Bivio delle urne | Abisso, livello 9"));
}
