#include "EidolonGameMode.h"
#include "EidolonCharacter.h"
#include "EidolonArenaEnemy.h"
#include "EidolonArenaHUD.h"
#include "EidolonCampaignSubsystem.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/DirectionalLight.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "GameFramework/PlayerController.h"

AEidolonGameMode::AEidolonGameMode()
{
    DefaultPawnClass = AEidolonCharacter::StaticClass();
    HUDClass = AEidolonArenaHUD::StaticClass();
}

void AEidolonGameMode::StartPlay()
{
    UWorld* World = GetWorld();
    auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    const auto Block = [World, Cube](FVector Position, FVector Scale)
    {
        if (!Cube) return;
        auto* Actor = World->SpawnActor<AStaticMeshActor>(Position, FRotator::ZeroRotator);
        if (!Actor) return;
        auto* Mesh = Actor->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Cube);
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        Actor->SetActorScale3D(Scale);
    };
    Block(FVector(0,0,-50), FVector(32,32,1));
    Block(FVector(0,1600,150), FVector(32,1,3));
    Block(FVector(0,-1600,150), FVector(32,1,3));
    Block(FVector(1600,0,150), FVector(1,32,3));
    Block(FVector(-1600,0,150), FVector(1,32,3));
    auto* Sun = World->SpawnActor<ADirectionalLight>(FVector(0,0,800), FRotator(-55,-30,0));
    if (Sun) Sun->GetLightComponent()->SetIntensity(8.f);
    Super::StartPlay();
    if (auto* PC = World->GetFirstPlayerController())
    {
        if (!PC->GetPawn()) RestartPlayer(PC);
        if (auto* Pawn = PC->GetPawn()) Pawn->SetActorLocation(FVector(0,0,110), false, nullptr, ETeleportType::TeleportPhysics);
        PC->SetControlRotation(FRotator(-15,0,0));
    }
    SpawnEncounter();
}

namespace
{
    UEidolonCampaignSubsystem* Campaign(const AActor* Actor)
    {
        auto* GI = Actor->GetGameInstance();
        return GI ? GI->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
    }
}

bool AEidolonGameMode::IsEncounterClear() const
{
    for (TActorIterator<AEidolonArenaEnemy> It(GetWorld()); It; ++It)
        if (!It->IsActorBeingDestroyed()) return false;
    return true;
}

void AEidolonGameMode::SpawnEncounter()
{
    auto* C = Campaign(this);
    if (!C || !C->GetEnding().IsNone()) return;
    const auto Chapter = C->CurrentChapter();
    int32 Count = FMath::Clamp(Chapter.Target - C->GetObjectiveCount(), 0, 3);
    if (Chapter.Id == 4 || Chapter.Id == 23)
        Count = C->GetFloor(Chapter.Id == 23) < 100 ? 1 : 0;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    for (int32 Index = 0; Index < Count; ++Index)
        GetWorld()->SpawnActor<AEidolonArenaEnemy>(FVector(850,(Index-1)*500,55), FRotator::ZeroRotator, Params);
}

bool AEidolonGameMode::CommitCheckpoint()
{
    auto* C = Campaign(this);
    if (C && C->SaveCampaign()) return true;
    Notice = TEXT("Checkpoint write failed. Progress remains in memory.");
    return false;
}

void AEidolonGameMode::Interact(AEidolonCharacter* Player)
{
    auto* C = Campaign(this);
    if (!C || !Player || Player->IsDefeated() || !C->GetEnding().IsNone()) return;
    const auto Chapter = C->CurrentChapter();
    if (!Chapter.Id) { Notice = TEXT("Campaign data missing."); return; }
    if (!IsEncounterClear()) { Notice = TEXT("Defeat the remaining enemies."); return; }
    if (Chapter.Id == 26) { Notice = TEXT("Choose your ending with 1-4 or D-pad."); return; }
    if (Chapter.Id == 4 || Chapter.Id == 23)
    {
        const bool bStair = Chapter.Id == 23;
        if (C->GetFloor(bStair) < 100)
        {
            C->ReachFloor(bStair, C->GetFloor(bStair)+1);
            if (C->GetFloor(bStair) < 100)
            {
                CommitCheckpoint();
                SpawnEncounter();
                return;
            }
            C->RecordObjective(Chapter.Type);
        }
    }
    if (Chapter.Target > C->GetObjectiveCount())
    {
        CommitCheckpoint();
        SpawnEncounter();
        return;
    }
    if (Chapter.Id == 22)
    {
        if (!C->HasFlag(TEXT("noah_disappeared")))
        {
            C->ApplyDiscrepancy(TEXT("noah_forgotten"), TEXT("noah_present"), false);
            C->SetFlag(TEXT("noah_disappeared"));
            Notice = TEXT("Noah has vanished. Follow the memory into Oblivion. Interact again.");
            CommitCheckpoint();
            return;
        }
        C->SetFlag(TEXT("noah_found_in_oblivion"));
    }
    if (Chapter.Id == 24) C->SetFlag(TEXT("origin_truth"));
    C->SetFlag(FName(*FString::Printf(TEXT("chapter_%d_resolved"), Chapter.Id)));
    if (!C->AdvanceChapter()) return;
    Notice.Reset();
    CommitCheckpoint();
    SpawnEncounter();
}

void AEidolonGameMode::SelectEnding(int32 Choice)
{
    auto* C = Campaign(this);
    if (!C || !IsEncounterClear()) return;
    const FName Choices[] = {TEXT("custodian"), TEXT("origin"), TEXT("guild"), TEXT("freedom")};
    if (Choice < 1 || Choice > 4) return;
    if (!C->ChooseEnding(Choices[Choice-1])) { Notice = TEXT("Ending unavailable: requirements not fulfilled."); return; }
    Notice.Reset();
    CommitCheckpoint();
}

void AEidolonGameMode::ReloadCheckpoint()
{
    auto* C = Campaign(this);
    if (C && !C->LoadCampaign()) C->NewCampaign();
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

FString AEidolonGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    auto* C = Campaign(this);
    if (!C) return TEXT("Campaign unavailable");
    if (!C->GetEnding().IsNone()) return TEXT("Campaign ended.");
    if (!IsEncounterClear()) return TEXT("Defeat enemies");
    if (C->CurrentChapter().Id == 26) return TEXT("1 Custodian | 2 Origin | 3 Guild | 4 Freedom (locked unless qualified)");
    return TEXT("E / X or Square: continue");
}
FString AEidolonGameMode::GetNarrative() const
{
    auto* C = Campaign(this);
    if (!C) return TEXT("");
    const auto Chapter = C->CurrentChapter();
    if (!C->GetEnding().IsNone()) return FString::Printf(TEXT("Ending: %s"), *C->GetEnding().ToString());
    return FString::Printf(TEXT("Act %d | Chapter %d: %s | %s"), Chapter.Act, Chapter.Id, *Chapter.Title, *Chapter.Region);
}
