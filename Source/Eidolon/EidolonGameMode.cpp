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
#include "Components/TextRenderComponent.h"
#include "Engine/TextRenderActor.h"
#include "Materials/MaterialInstanceDynamic.h"
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
    BuildChapterScenery();
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
    {
        auto* Enemy = GetWorld()->SpawnActor<AEidolonArenaEnemy>(FVector(850,(Index-1)*500,55), FRotator::ZeroRotator, Params);
        if (Enemy)
        {
            const bool bFloor = Chapter.Id == 4 || Chapter.Id == 23;
            const int32 Difficulty = bFloor ? C->GetFloor(Chapter.Id == 23)+1 : Chapter.Id;
            Enemy->Configure(bFloor && Difficulty == 100 ? FName(TEXT("boss")) : Chapter.Type, Difficulty);
        }
    }
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
    Notice.Reset();
    const auto Chapter = C->CurrentChapter();
    if (!Chapter.Id) { Notice = TEXT("Campaign data missing."); return; }
    if (!IsEncounterClear()) { Notice = TEXT("Defeat the remaining enemies."); return; }
    if (DialogueIndex < Chapter.Dialogue.Num()) { ++DialogueIndex; Notice.Reset(); return; }
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
    DialogueIndex = 0;
    BuildChapterScenery();
    CommitCheckpoint();
    SpawnEncounter();
}

void AEidolonGameMode::SelectEnding(int32 Choice)
{
    auto* C = Campaign(this);
    if (!C || !IsEncounterClear() || DialogueIndex < C->CurrentChapter().Dialogue.Num()) return;
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
    if (C->CurrentChapter().Id == 26 && DialogueIndex >= C->CurrentChapter().Dialogue.Num()) return TEXT("1 Custodian | 2 Origin | 3 Guild | 4 Freedom (locked unless qualified)");
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

FString AEidolonGameMode::GetDialogue() const
{
    auto* C = Campaign(this);
    if (!C) return TEXT("");
    if (!C->GetEnding().IsNone()) return C->GetEpilogue();
    const auto Chapter = C->CurrentChapter();
    if (DialogueIndex > 0 && Chapter.Dialogue.IsValidIndex(DialogueIndex-1))
        return Chapter.Dialogue[DialogueIndex-1];
    return TEXT("");
}
void AEidolonGameMode::RecoverMemory(AEidolonCharacter* Player)
{
    auto* C = Campaign(this);
    if (!C || !Player || Player->IsDefeated() || !IsEncounterClear() || !C->GetEnding().IsNone()) return;
    const int32 Chapter = C->CurrentChapter().Id;
    if (Chapter == 6 && !C->HasFlag(TEXT("lost_memories")))
    {
        C->RecordMemory(TEXT("edran_diary"));
        C->SetFlag(TEXT("lost_memories"));
        Notice = TEXT("Edran's diary recovered.");
        CommitCheckpoint();
    }
    else if (Chapter == 20 && !C->HasFlag(TEXT("eidolon_aid")))
    {
        C->SetFlag(TEXT("eidolon_aid"));
        Notice = TEXT("Forgotten residents assisted (graybox objective).");
        CommitCheckpoint();
    }
}

void AEidolonGameMode::BuildChapterScenery()
{
    for (const auto& Actor : Scenery) if (IsValid(Actor.Get())) Actor->Destroy();
    Scenery.Reset();
    auto* C = Campaign(this);
    if (!C) return;
    const auto Chapter = C->CurrentChapter();
    auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
    const auto Shape = [this](UStaticMesh* Mesh, FVector P, FVector S)
    {
        if (!Mesh) return;
        auto* A = GetWorld()->SpawnActor<AStaticMeshActor>(P, FRotator::ZeroRotator);
        if (!A) return;
        A->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        A->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        A->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("NoCollision"));
        A->SetActorScale3D(S);
        Scenery.Add(A);
    };
    const auto Label = [this](const FString& Text, FVector P)
    {
        auto* A = GetWorld()->SpawnActor<ATextRenderActor>(P, FRotator(0,180,0));
        if (!A) return;
        auto* T = A->GetTextRender();
        T->SetText(FText::FromString(Text));
        T->SetWorldSize(36.f);
        T->SetHorizontalAlignment(EHTA_Center);
        Scenery.Add(A);
    };
    if (Chapter.Type == TEXT("home"))
    {
        Shape(Cube, FVector(1150,0,180), FVector(1,14,3.6));
        Shape(Cube, FVector(850,650,180), FVector(7,1,3.6));
        Shape(Cube, FVector(850,-650,180), FVector(7,1,3.6));
        Shape(Cube, FVector(900,0,65), FVector(2,3,1.3));
        Label(TEXT("Darius / Elara / Lyra"), FVector(1100,0,340));
        if (C->HasFlag(TEXT("noah_present")))
        {
            Shape(Cylinder,FVector(600,200,55),FVector(.55,.55,1.1));
            Label(TEXT("Noah"), FVector(600,200,140));
        }
    }
    else if (Chapter.Type == TEXT("dungeon"))
    {
        for (int32 I=0; I<5; ++I)
            for (int32 Side : {-1,1})
            {
                Shape(Cylinder,FVector(-1100+I*550,Side*1050,250), FVector(1.5,1.5,5));
                Shape(Cube,FVector(-1100+I*550,Side*1050,500), FVector(2,2,.5));
            }
        Label(Chapter.Region,FVector(1200,0,300));
    }
    else if (Chapter.Type == TEXT("giant") || Chapter.Type == TEXT("titan"))
    {
        for (int32 I=0; I<6; ++I)
        {
            Shape(Cylinder,FVector(-1250+I*480,1250,180),FVector(.6,.6,3.6));
            Shape(Cone,FVector(-1250+I*480,1250,420),FVector(2.5,2.5,5));
        }
        Label(Chapter.Region,FVector(1200,0,300));
    }
    else
    {
        for (int32 I=0; I<4; ++I)
            Shape(Cube,FVector(1300,-1050+I*700,180+I*35),FVector(2.5,3.5,3.6+I*.7));
        Label(Chapter.Region,FVector(1200,0,330));
    }
}
