#include "EidolonRattleMillipede.h"

#include "../EidolonCharacter.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr float SideStrikeDamage = 18.0f;
    constexpr double PatrolSpeedCm = 135.0;
    constexpr double PatrolMaxStepCm = 30.0;
    constexpr double WaypointToleranceCm = 12.0;
}

AEidolonRattleMillipede::AEidolonRattleMillipede()
{
    SetActorScale3D(FVector(1.13, 0.84, 0.63));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FName PlateName(*FString::Printf(TEXT("RattlePlate%d"), Index));
        UStaticMeshComponent* Plate = CreateDefaultSubobject<UStaticMeshComponent>(PlateName);
        Plate->SetupAttachment(GetRootComponent());
        Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Plate->SetGenerateOverlapEvents(false);
        if (Sphere.Succeeded()) Plate->SetStaticMesh(Sphere.Object);
        const double Taper = 1.0 - 0.12 * Index;
        Plate->SetRelativeScale3D(FVector(0.54, 0.77 * Taper, 0.64 * Taper));
        Plates.Add(Plate);
    }

    Telegraph = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Telegraph"));
    Telegraph->SetupAttachment(GetRootComponent());
    Telegraph->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Telegraph->SetGenerateOverlapEvents(false);
    Telegraph->SetCastShadow(false);
    if (Cube.Succeeded()) Telegraph->SetStaticMesh(Cube.Object);
    UpdateTelegraph();
}

void AEidolonRattleMillipede::BeginPlay()
{
    Super::BeginPlay();
    Configure(TEXT("abyss_rattle_millipede"), 6);
}

bool AEidolonRattleMillipede::SetPatrolRoute(const TArray<FVector>& Points)
{
    if (!HasAuthority() || Points.Num() < 2 || Attack.Current != EidolonRattleMillipede::Phase::Ready ||
        FVector::DistSquared(Points[0], GetActorLocation()) > FMath::Square(WaypointToleranceCm))
    {
        return false;
    }

    for (int32 Index = 0; Index < Points.Num(); ++Index)
    {
        const FVector& From = Points[Index];
        const FVector& To = Points[(Index + 1) % Points.Num()];
        if (From.ContainsNaN() || !HasGroundSupport(From)
            || FVector::DistSquared(From, To) <= FMath::Square(WaypointToleranceCm))
        {
            return false;
        }
    }

    Patrol = Points;
    LastWaypoint = 0;
    NextWaypoint = 1;
    bForward = true;
    PatrolPause = 0.0;
    return true;
}

bool AEidolonRattleMillipede::HasGroundSupport(const FVector& Location) const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    const FCollisionQueryParams Query(SCENE_QUERY_STAT(RattleMillipedeGround), false, this);
    const FCollisionObjectQueryParams Surfaces(ECC_WorldStatic);
    FHitResult Hit;
    return World->LineTraceSingleByObjectType(Hit, Location, Location - FVector(0.0, 0.0, 65.0), Surfaces, Query)
        && Hit.ImpactNormal.Z > 0.5;
}

void AEidolonRattleMillipede::AdvancePatrol(double DeltaSeconds)
{
    if (Patrol.IsEmpty()) return;
    if (PatrolPause > 0.0)
    {
        PatrolPause = FMath::Max(0.0, PatrolPause - DeltaSeconds);
        return;
    }

    const FVector Offset = Patrol[NextWaypoint] - GetActorLocation();
    const double Distance = Offset.Size();
    if (Distance <= WaypointToleranceCm)
    {
        LastWaypoint = NextWaypoint;
        NextWaypoint = static_cast<int32>(EidolonRattleMillipede::AdjacentWaypoint(LastWaypoint, Patrol.Num(), bForward));
        return;
    }

    const double Step = FMath::Min(Distance, FMath::Min(PatrolMaxStepCm, PatrolSpeedCm * DeltaSeconds));
    const FVector Movement = Offset.GetSafeNormal() * Step;
    if (!HasGroundSupport(GetActorLocation() + Movement))
    {
        ReversePatrol();
        return;
    }

    FHitResult Blocker;
    AddActorWorldOffset(Movement, true, &Blocker);
    if (Blocker.bBlockingHit)
    {
        ReversePatrol();
        return;
    }

    const FVector Facing = Movement.GetSafeNormal2D();
    if (!Facing.IsNearlyZero()) SetActorRotation(Facing.Rotation());
}

void AEidolonRattleMillipede::ReversePatrol()
{
    bForward = !bForward;
    NextWaypoint = LastWaypoint;
    PatrolPause = 0.40;
}

bool AEidolonRattleMillipede::HasSightOf(const AEidolonCharacter& Player) const
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(RattleMillipedeStrike), false, this);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Player.GetActorLocation(), ECC_Visibility, Query)
        || Hit.GetActor() == &Player;
}

void AEidolonRattleMillipede::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;

    using namespace EidolonRattleMillipede;
    const Phase Previous = Attack.Current;
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const bool PlayerAvailable = Player && !Player->IsDefeated();
    const FVector Offset = PlayerAvailable ? Player->GetActorLocation() - GetActorLocation() : FVector::ZeroVector;
    const double Along = FVector::DotProduct(Offset, GetActorForwardVector());
    const double Side = FVector::DotProduct(Offset, GetActorRightVector());
    if (Previous == Phase::Ready)
    {
        if (PlayerAvailable && Attack.Begin(HasSightOf(*Player), Along, Side, Offset.Z))
            UpdateTelegraph();
        else
            AdvancePatrol(DeltaSeconds);
        return;
    }

    if (Previous == Phase::Strike && PlayerAvailable && !Attack.bSpent
        && Attack.TryHit(HasSightOf(*Player), Along, Side, Offset.Z))
    {
        UGameplayStatics::ApplyDamage(Player, SideStrikeDamage, nullptr, this, nullptr);
    }

    Attack.AdvanceTime(DeltaSeconds);
    if (Attack.Current != Previous) UpdateTelegraph();
}

void AEidolonRattleMillipede::UpdateTelegraph()
{
    using namespace EidolonRattleMillipede;
    const bool Windup = Attack.Current == Phase::Telegraph;
    const bool Striking = Attack.Current == Phase::Strike;
    const double Opening = Striking ? 78.0 : Windup ? 52.0 : 0.0;
    for (int32 Index = 0; Index < Plates.Num(); ++Index)
    {
        Plates[Index]->SetRelativeLocation(FVector(-37.0 - 37.0 * Index, (Index % 2 == 0 ? 1.0 : -1.0) * 8.0, 0.0));
        Plates[Index]->SetRelativeRotation(FRotator(0.0, 0.0, Attack.LockedSide * Opening));
    }

    Telegraph->SetHiddenInGame(!Windup && !Striking);
    if (!Windup && !Striking) return;

    const double Thickness = Striking ? 8.0 : 2.0;
    const FVector Center = GetActorLocation() + GetActorForwardVector() * ((NoseCm + TailCm) / 2.0)
        + GetActorRightVector() * (Attack.LockedSide * ((OuterSideCm + InnerSideCm) / 2.0))
        + FVector(0.0, 0.0, -36.0 + Thickness / 2.0);
    Telegraph->SetWorldLocationAndRotation(Center, GetActorRotation());
    Telegraph->SetWorldScale3D(FVector(NoseCm - TailCm, OuterSideCm - InnerSideCm, Thickness) / 100.0);
}

float AEidolonRattleMillipede::TakeDamage(float Amount, const FDamageEvent& Event,
                                          AController* Instigator, AActor* Causer)
{
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!HasAuthority() || !Player || Player->IsDefeated() || Causer != Player || !Instigator
        || Player->GetController() != Instigator || !FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return 0.0f;
    }

    return Super::TakeDamage(Amount, Event, Instigator, Causer);
}
