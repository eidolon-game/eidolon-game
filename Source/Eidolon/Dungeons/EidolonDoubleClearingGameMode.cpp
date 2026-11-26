#include "EidolonDoubleClearingGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonForestStalker.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"

FEidolonAbyssEncounter AEidolonDoubleClearingGameMode::GetEncounter() const
{
    using namespace EidolonAbyss;
    FEidolonAbyssEncounter Floor;
    Floor.Floor = DoubleClearingFloor;
    Floor.GateTag = TEXT("abyss_018_gate");
    Floor.TitleEnglish = TEXT("Double Clearing | Abyss, floor 18");
    Floor.TitleItalian = TEXT("Doppia radura | Abisso, livello 18");
    Floor.FightEnglish = TEXT("Follow the side path and raise the latch.");
    Floor.FightItalian = TEXT("Segui il sentiero laterale e solleva il chiavistello.");
    Floor.CompletionKind = EventKind::DoubleClearingLatchOpened;
    Floor.CompletionTarget = Target::DoubleClearingLatch;
    Floor.CompletionControlTag = TEXT("abyss_018_latch");
    return Floor;
}

bool AEidolonDoubleClearingGameMode::BindCompletionActors(UWorld& World)
{
    LatchHandle = EidolonAbyss::FindTaggedActor<AStaticMeshActor>(World, TEXT("abyss_018_latch_handle"));
    auto* Handle = LatchHandle.Get();
    if (!Handle) return false;

    RestPose = Handle->GetActorRotation();
    Handle->SetActorEnableCollision(false);
    return true;
}

bool AEidolonDoubleClearingGameMode::TryCompleteControl(UEidolonCampaignSubsystem& Campaign,
    AEidolonCharacter& Player, const ATargetPoint& Control)
{
    if (!LatchHandle.IsValid() || !CanInteractAt(Player, &Control)) return false;

    const uint8 Step = Campaign.GetAbyssDoubleClearingLatchStep();
    if (!Campaign.CommitAbyssDoubleClearingLatchStep(Step))
    {
        ReportCheckpointSaveFailure();
        return false;
    }

    ClearNotice();
    if (Campaign.GetAbyssDoubleClearingLatchStep() == EidolonAbyss::DoubleClearingLatchMaxStep) return true;

    UpdateCompletionActors(false);
    return false;
}

void AEidolonDoubleClearingGameMode::UpdateCompletionActors(bool)
{
    auto* Handle = LatchHandle.Get();
    const auto* Campaign = GetCampaign();
    if (!Handle || !Campaign) return;

    const uint8 Step = CanUseFloor(*Campaign) ? Campaign->GetAbyssDoubleClearingLatchStep() : 0;
    const FQuat Pivot = RestPose.Quaternion();
    constexpr double AnglePerDetent = 28.0;
    const FQuat Detent(FVector::RightVector, FMath::DegreesToRadians(-AnglePerDetent * Step));
    Handle->SetActorRotation(Pivot * Detent, ETeleportType::TeleportPhysics);
}

FString AEidolonDoubleClearingGameMode::GetControlPrompt() const
{
    const auto* Campaign = GetCampaign();
    if (!Campaign || Campaign->GetAbyssDoubleClearingLatchStep() != 1) return Super::GetControlPrompt();
    return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
        ? TEXT("Fermo inserito. Libera la paratia.") : TEXT("Catch engaged. Release the gate.");
}

void AEidolonDoubleClearingGameMode::SpawnOptionalThreats()
{
    using EidolonAbyss::FindTaggedActor;
    auto* World = GetWorld();
    if (!World) return;

    const auto SpawnAt = [World](const ATargetPoint* Anchor) -> AEidolonForestStalker*
    {
        if (!Anchor) return nullptr;
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        return World->SpawnActor<AEidolonForestStalker>(Anchor->GetActorLocation() + FVector(0, 0, 95.0),
            Anchor->GetActorRotation(), Params);
    };

    auto* Ground = SpawnAt(FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_018_stalker_ground")));
    const auto* Perch = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_018_stalker_perch"));
    const auto* Landing = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_018_stalker_landing"));
    auto* High = Perch && Landing ? SpawnAt(Perch) : nullptr;
    if (High && !High->ConfigurePerch(Perch->GetActorLocation(), Landing->GetActorLocation(), true))
    {
        High->Destroy();
        High = nullptr;
    }
    if (!High) High = SpawnAt(FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_018_stalker_high")));

    if (Ground && High)
    {
        Ground->SetAttackPartner(High);
        High->SetAttackPartner(Ground);
    }
}
