#include "EidolonArenaEnemy.h"
#include "EidolonCharacter.h"
#include "EidolonCampaignSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AEidolonArenaEnemy::AEidolonArenaEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    SetRootComponent(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Mesh.Succeeded()) Body->SetStaticMesh(Mesh.Object);
    Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Body->SetGenerateOverlapEvents(false);
}

void AEidolonArenaEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Player || Player->IsDefeated()) return;
    AttackCooldown = FMath::Max(0.f, AttackCooldown - DeltaSeconds);
    FVector Offset = Player->GetActorLocation() - GetActorLocation();
    Offset.Z = 0.f;
    const float Distance = Offset.Size();
    if (Distance > 140.f)
        AddActorWorldOffset(Offset.GetSafeNormal() * FMath::Min(180.f * DeltaSeconds, Distance - 140.f), true);
    else if (FMath::Abs(Player->GetActorLocation().Z - GetActorLocation().Z) < 150.f && AttackCooldown <= 0.f)
    {
        UGameplayStatics::ApplyDamage(Player, 12.f, nullptr, this, nullptr);
        AttackCooldown = 1.25f;
    }
}

float AEidolonArenaEnemy::TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer)
{
    if (Amount <= 0.f || Health <= 0.f) return 0.f;
    const float Applied = FMath::Min(Health, Amount);
    Health -= Applied;
    if (Health <= 0.f)
    {
        if (auto* GI = GetGameInstance())
            if (auto* C = GI->GetSubsystem<UEidolonCampaignSubsystem>())
            {
                const auto Chapter = C->CurrentChapter();
                if (Chapter.Id != 4 && Chapter.Id != 23) C->RecordObjective(Chapter.Type);
            }
        Destroy();
    }
    return Applied;
}
