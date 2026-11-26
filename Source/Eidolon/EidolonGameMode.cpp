#include "EidolonGameMode.h"
#include "EidolonCharacter.h"
#include "EidolonArenaEnemy.h"
#include "EidolonArenaHUD.h"
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
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    for (const FVector Position : {FVector(850,0,55), FVector(850,650,55), FVector(850,-650,55)})
        World->SpawnActor<AEidolonArenaEnemy>(Position, FRotator::ZeroRotator, Params);
}
