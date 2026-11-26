#include "EidolonQuarryThresholdGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonMineBorer.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

namespace
{
    const FName ArrivalFlag = TEXT("abyss_quarry_arrived");
    constexpr double ProtectedArrivalEndX = 575.0;
}

FEidolonAbyssEncounter AEidolonQuarryThresholdGameMode::GetEncounter() const
{
    using namespace EidolonAbyss;
    FEidolonAbyssEncounter Floor;
    Floor.Floor = QuarryThresholdFloor;
    Floor.GateTag = TEXT("abyss_021_gate");
    Floor.TitleEnglish = TEXT("Quarry Threshold | Abyss, floor 21");
    Floor.TitleItalian = TEXT("Soglia di cava | Abisso, livello 21");
    Floor.FightEnglish = TEXT("Watch the Borer lower its head. Use a recess and reach the latch inside the gate.");
    Floor.FightItalian = TEXT("Osserva il Perforatore quando abbassa la testa. Riparati in una nicchia e raggiungi il chiavistello interno.");
    Floor.CompletionKind = EventKind::QuarryLatchOpened;
    Floor.CompletionTarget = Target::QuarryLatch;
    Floor.CompletionControlTag = TEXT("abyss_021_quarry_latch");
    return Floor;
}

bool AEidolonQuarryThresholdGameMode::CanUseStart(const APlayerStart& Start) const
{
    const auto* World = GetWorld();
    if (!World) return false;

    const auto* Capsule = GetDefault<AEidolonCharacter>()->GetCapsuleComponent();
    const double Radius = Capsule->GetUnscaledCapsuleRadius();
    const double HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
    const FVector Position = Start.GetActorLocation();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(QuarryArrival), false, &Start);
    if (World->OverlapBlockingTestByProfile(Position, FQuat::Identity, Capsule->GetCollisionProfileName(),
                                            FCollisionShape::MakeCapsule(Radius, HalfHeight), Query))
        return false;

    const FVector Foot = Position - FVector(0.0, 0.0, HalfHeight);
    FHitResult Ground;
    return World->LineTraceSingleByChannel(Ground, Foot + FVector(0, 0, 3.0), Foot - FVector(0, 0, 32.0),
                                           ECC_Visibility, Query)
        && Ground.ImpactNormal.Z >= .9;
}

void AEidolonQuarryThresholdGameMode::RestartPlayer(AController* Controller)
{
    auto* World = GetWorld();
    if (!IsValid(Controller) || Controller->GetPawn() || !World) return;
    const auto* Campaign = GetCampaign();
    const bool bCheckpoint = UGameplayStatics::HasOption(OptionsString, TEXT("AbyssCheckpoint"))
        || (Campaign && Campaign->HasFlag(ArrivalFlag));
    const FName First = bCheckpoint ? TEXT("abyss_021_checkpoint_start") : TEXT("abyss_021_player_start");
    const FName Recovery = TEXT("abyss_021_arrival_recovery");
    for (const FName Tag : {First, Recovery})
    {
        auto* Start = EidolonAbyss::FindTaggedActor<APlayerStart>(*World, Tag);
        if (!Start || !CanUseStart(*Start)) continue;
        RestartPlayerAtPlayerStart(Controller, Start);
        if (auto* Player = Cast<AEidolonCharacter>(Controller->GetPawn()))
        {
            Controller->SetControlRotation(FRotator(-10.0, Start->GetActorRotation().Yaw, 0.0));
            if (Campaign && CanUseFloor(*Campaign) && ConfirmArrival(*Player)) SpawnOptionalThreats();
            return;
        }
    }

    if (!bFallbackQueued && Campaign && CanUseFloor(*Campaign))
    {
        bFallbackQueued = true;
        GetWorldTimerManager().SetTimerForNextTick(this, &AEidolonQuarryThresholdGameMode::ReturnToRiftroot);
    }
}

bool AEidolonQuarryThresholdGameMode::ConfirmArrival(AEidolonCharacter& Player)
{
    auto* Campaign = GetCampaign();
    const auto* World = GetWorld();
    const auto* Controller = World ? World->GetFirstPlayerController() : nullptr;
    const FVector Position = Player.GetActorLocation();
    if (!Campaign || !CanUseFloor(*Campaign) || !Controller || Controller->GetPawn() != &Player
        || Player.IsDefeated() || Position.X < 0 || Position.X >= ProtectedArrivalEndX
        || Position.Z < 92 || Position.Z > 135)
        return false;

    if (Campaign->CommitQuarryArrival())
    {
        ClearNotice();
        return true;
    }
    ReportCheckpointSaveFailure();
    return false;
}

void AEidolonQuarryThresholdGameMode::ReturnToRiftroot()
{
    const auto* World = GetWorld();
    const auto* Player = World ? World->GetFirstPlayerController() : nullptr;
    if (Player && Player->GetPawn())
    {
        bFallbackQueued = false;
        return;
    }
    const FName Previous = TEXT("/Game/Eidolon/Maps/AbyssFloor020");
    if (FPackageName::DoesPackageExist(Previous.ToString()))
        UGameplayStatics::OpenLevel(this, Previous, true, TEXT("AbyssCheckpoint"));
    else
        UE_LOG(LogTemp, Error, TEXT("Quarry arrivals are blocked and the Riftroot recovery map is unavailable."));
}

void AEidolonQuarryThresholdGameMode::Interact(AEidolonCharacter* Player)
{
    const auto* Campaign = GetCampaign();
    if (Campaign && CanUseFloor(*Campaign) && !Campaign->HasFlag(ArrivalFlag))
    {
        if (IsValid(Player) && ConfirmArrival(*Player)) SpawnOptionalThreats();
        return;
    }
    Super::Interact(Player);
}

bool AEidolonQuarryThresholdGameMode::BindCompletionActors(UWorld& World)
{
    using EidolonAbyss::FindTaggedActor;
    QuarryGate = FindTaggedActor<AStaticMeshActor>(World, GetEncounter().GateTag);
    BorerStart = FindTaggedActor<ATargetPoint>(World, TEXT("abyss_021_borer_spawn"));
    ChargeLimit = FindTaggedActor<ATargetPoint>(World, TEXT("abyss_021_charge_limit"));
    return QuarryGate.IsValid() && BorerStart.IsValid() && ChargeLimit.IsValid()
        && FindTaggedActor<APlayerStart>(World, TEXT("abyss_021_arrival_recovery"));
}

bool AEidolonQuarryThresholdGameMode::InteractWithControl(AEidolonCharacter& Player, const ATargetPoint& Control)
{
    const auto* Gate = QuarryGate.Get();
    return Gate && Player.GetActorLocation().X < Gate->GetActorLocation().X
        && Control.GetActorLocation().X < Gate->GetActorLocation().X && CanInteractAt(Player, &Control);
}

void AEidolonQuarryThresholdGameMode::SpawnOptionalThreats()
{
    const auto* Campaign = GetCampaign();
    auto* World = GetWorld();
    const auto* Start = BorerStart.Get();
    const auto* Limit = ChargeLimit.Get();
    if (bThreatSpawned || !World || !Start || !Limit || !Campaign || !Campaign->HasFlag(ArrivalFlag)
        || Campaign->GetAuthoredAbyssCompletedFloor() >= EidolonAbyss::QuarryThresholdFloor)
        return;

    const FVector Lift(0.0, 0.0, EidolonMineBorer::BodyHalfHeightCm + 2.0);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    if (auto* Borer = World->SpawnActor<AEidolonMineBorer>(Start->GetActorLocation() + Lift, Start->GetActorRotation(), Params))
    {
        if (Borer->ConfigureChargeLane(Limit->GetActorLocation() + Lift))
            bThreatSpawned = true;
        else
            Borer->Destroy();
    }
}
