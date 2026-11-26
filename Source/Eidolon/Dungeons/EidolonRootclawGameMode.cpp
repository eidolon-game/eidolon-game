#include "EidolonRootclawGameMode.h"

#include "EidolonAbyssActorBindings.h"
#include "../EidolonCharacter.h"
#include "EidolonRootclaw.h"
#include "EidolonVaultPincer.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    constexpr double RootclawHeightCm = 135.0;
    constexpr double CourtyardCrawlerRangeCm = 650.0;

    template <typename EnemyType>
    void SpawnCourtyardEnemy(UWorld& World, FName Tag, double Height)
    {
        const auto* Anchor = EidolonAbyss::FindTaggedActor<ATargetPoint>(World, Tag);
        if (Anchor)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
            if (World.SpawnActor<EnemyType>(
                    Anchor->GetActorLocation() + FVector(0, 0, Height), Anchor->GetActorRotation(), Params))
            {
                return;
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("Optional courtyard encounter could not spawn at %s."), *Tag.ToString());
    }
}

FEidolonAbyssEncounter AEidolonRootclawGameMode::GetEncounter() const
{
    return {
        EidolonAbyss::RootclawFloor, TEXT("abyss_010_rootclaw_01"), TEXT("abyss_010_rootclaw_gate"),
        AEidolonRootclaw::StaticClass(), RootclawHeightCm, false,
        TEXT("Root Antechamber | Abyss, floor 10"), TEXT("Anticamera delle radici | Abisso, livello 10"),
        TEXT("Watch the claw and leave the signaled lane."), TEXT("Osserva la chela ed esci dalla corsia segnalata."),
        EidolonAbyss::EventKind::RootclawDefeated, EidolonAbyss::Target::Rootclaw
    };
}

bool AEidolonRootclawGameMode::ConfigureEnemy(AEidolonArenaEnemy& Enemy)
{
    auto* Boss = Cast<AEidolonRootclaw>(&Enemy);
    auto* World = GetWorld();
    if (!Boss || !World) return false;

    const FName LaneTags[] = {TEXT("abyss_010_lane_left"), TEXT("abyss_010_lane_center"), TEXT("abyss_010_lane_right")};
    TArray<FVector> Lanes;
    Lanes.Reserve(UE_ARRAY_COUNT(LaneTags));
    for (FName Tag : LaneTags)
    {
        const auto* Anchor = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, Tag);
        if (!Anchor) return false;
        Lanes.Add(Anchor->GetActorLocation() + FVector(0, 0, RootclawHeightCm));
    }

    return Boss->SetLanePositions(Lanes);
}

void AEidolonRootclawGameMode::SpawnOptionalThreats()
{
    auto& World = *GetWorld();
    SpawnCourtyardEnemy<AEidolonRootclawCourtyardCrawler>(World, TEXT("abyss_010_catacomb_crawler_01"), 55.0);
    SpawnCourtyardEnemy<AEidolonVaultPincer>(World, TEXT("abyss_010_vault_pincer_01"), 50.0);
}

void AEidolonRootclawCourtyardCrawler::BeginPlay()
{
    Super::BeginPlay();
    SpawnPosition = GetActorLocation();
}

void AEidolonRootclawCourtyardCrawler::Tick(float DeltaSeconds)
{
    const auto* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (Player && FVector::DistSquared2D(Player->GetActorLocation(), SpawnPosition) <=
        FMath::Square(CourtyardCrawlerRangeCm))
    {
        Super::Tick(DeltaSeconds);
    }
    else
    {
        AActor::Tick(DeltaSeconds);
    }
}
