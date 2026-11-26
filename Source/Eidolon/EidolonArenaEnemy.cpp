#include "EidolonArenaEnemy.h"
#include "EidolonCharacter.h"
#include "EidolonCampaignSubsystem.h"
#include "EidolonMeleeReach.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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

void AEidolonArenaEnemy::Configure(FName Type, int32 Difficulty)
{
    ObjectiveType = Type;
    if (auto* GI = GetGameInstance())
        if (auto* C = GI->GetSubsystem<UEidolonCampaignSubsystem>()) OriginChapter = C->CurrentChapter().Id;
    const float Tier = FMath::Clamp(Difficulty, 1, 100) / 100.f;
    Health = 60.f + 40.f * Tier;
    if (Type == TEXT("giant")) { Health = 130.f; Damage = 24.f; Speed = 120.f; Reach = 210.f; Recovery = 2.f; WindupDuration = .9f; SetActorScale3D(FVector(2.f)); }
    else if (Type == TEXT("titan") || Type == TEXT("boss")) { Health = 280.f; Damage = 32.f; Speed = 105.f; Reach = 260.f; Recovery = 2.3f; WindupDuration = 1.1f; SetActorScale3D(FVector(2.5f)); }
}

void AEidolonArenaEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Player || Player->IsDefeated()) return;
    AttackCooldown = FMath::Max(0.f, AttackCooldown - DeltaSeconds);
    const FVector Offset = Player->GetActorLocation() - GetActorLocation();
    const double Distance = Offset.Size2D();
    const bool bWithinReach = EidolonMelee::IsWithinReach(Offset, Reach);
    if (bWindingUp)
    {
        Windup -= DeltaSeconds;
        if (Windup > 0.f) return;
        bWindingUp = false;
        AttackCooldown = Recovery;
        if (bWithinReach)
        {
            FHitResult Hit;
            FCollisionQueryParams Params;
            Params.AddIgnoredActor(this);
            if (!GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Player->GetActorLocation(), ECC_Visibility, Params) || Hit.GetActor() == Player)
                UGameplayStatics::ApplyDamage(Player, Damage, nullptr, this, nullptr);
        }
        return;
    }
    if (Distance > Reach)
    {
        const double Step = FMath::Min(static_cast<double>(Speed) * DeltaSeconds, Distance - Reach);
        AddActorWorldOffset(Offset.GetSafeNormal2D() * Step, true);
    }
    else if (bWithinReach && AttackCooldown <= 0.f)
    {
        bWindingUp = true;
        Windup = WindupDuration;
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
                if (Chapter.Id == OriginChapter && Chapter.Id != 4 && Chapter.Id != 23) C->RecordObjective(ObjectiveType);
            }
        Destroy();
    }
    return Applied;
}
