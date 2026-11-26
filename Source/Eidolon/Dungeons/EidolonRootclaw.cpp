#include "EidolonRootclaw.h"

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
    const FVector BodyScale(3.4, 2.65, 2.6);
    constexpr double SpawnLaneToleranceCm = 35.0;
    constexpr double MinLaneSpacingCm = 400.0;
    constexpr double GroundMarkerDropCm = 132.0;
    constexpr float SweepDamage = 32.0f;
    constexpr float ThrustDamage = 26.0f;
}

AEidolonRootclaw::AEidolonRootclaw()
{
    SetActorScale3D(BodyScale);
    HeavyClaw = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeavyClaw"));
    LightClaw = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LightClaw"));
    GroundMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMarker"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (UStaticMeshComponent* Part : {HeavyClaw.Get(), LightClaw.Get(), GroundMarker.Get()})
    {
        Part->SetupAttachment(GetRootComponent());
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
    }
    if (Cone.Succeeded())
    {
        HeavyClaw->SetStaticMesh(Cone.Object);
        LightClaw->SetStaticMesh(Cone.Object);
    }
    if (Cube.Succeeded()) GroundMarker->SetStaticMesh(Cube.Object);
    GroundMarker->SetCastShadow(false);
    GroundMarker->SetHiddenInGame(true);
}

void AEidolonRootclaw::BeginPlay()
{
    Super::BeginPlay();
    Configure(TEXT("boss"), 10);
    SetActorScale3D(BodyScale);
    StartingHealth = GetHealth();
    UpdatePose();
}

bool AEidolonRootclaw::SetLanePositions(const TArray<FVector>& Positions)
{
    if (!HasAuthority() || Positions.Num() != 3 || Attack.Current != EidolonRootclaw::Phase::Ready)
        return false;

    bool NearLane = false;
    for (const FVector& Position : Positions)
    {
        if (Position.ContainsNaN() || FMath::Abs(Position.Z - GetActorLocation().Z) > 5.0)
            return false;
        NearLane |= FVector::DistSquared(Position, GetActorLocation()) <= FMath::Square(SpawnLaneToleranceCm);
    }

    const FVector First = Positions[1] - Positions[0];
    const FVector Next = Positions[2] - Positions[1];
    if (!NearLane || First.Size2D() < MinLaneSpacingCm || Next.Size2D() < MinLaneSpacingCm
        || FVector::DotProduct(First.GetSafeNormal2D(), Next.GetSafeNormal2D()) < 0.99)
    {
        return false;
    }

    Lanes = Positions;
    return true;
}

bool AEidolonRootclaw::TryBeginLaneStep(const AEidolonCharacter& Player)
{
    if (Lanes.Num() != 3) return false;

    const FVector Position = GetActorLocation();
    const FVector PlayerPosition = Player.GetActorLocation();
    int32 Closest = 0;
    for (int32 Index = 1; Index < Lanes.Num(); ++Index)
    {
        if (FVector::DistSquared(Position, Lanes[Index]) < FVector::DistSquared(Position, Lanes[Closest]))
            Closest = Index;
    }

    int32 Destination = Closest == 0 ? 1 : Closest - 1;
    if (Closest == 1 && FVector::DistSquared(PlayerPosition, Lanes[2]) > FVector::DistSquared(PlayerPosition, Lanes[0]))
        Destination = 2;

    if (!Attack.Begin(EidolonRootclaw::Move::LaneStep, true)) return false;
    LaneDestination = Lanes[Destination];
    const FVector Facing = (LaneDestination - Position).GetSafeNormal2D();
    if (!Facing.IsNearlyZero()) SetActorRotation(Facing.Rotation());
    AttacksSinceLane = 0;
    return true;
}

bool AEidolonRootclaw::HasSightOf(const AEidolonCharacter& Player) const
{
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonRootclaw), false, this);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Player.GetActorLocation(), ECC_Visibility, Query)
        || Hit.GetActor() == &Player;
}

void AEidolonRootclaw::AimAt(const AEidolonCharacter& Player)
{
    const FVector Facing = (Player.GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (!Facing.IsNearlyZero()) SetActorRotation(Facing.Rotation());
}

void AEidolonRootclaw::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;

    using namespace EidolonRootclaw;
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const bool PlayerAvailable = Player && !Player->IsDefeated();
    const Phase Previous = Attack.Current;

    if (Previous == Phase::Ready)
    {
        if (!PlayerAvailable) return;
        if (Attack.bTransitionPending || (Attack.bSecondPhase && AttacksSinceLane >= 2))
        {
            if (TryBeginLaneStep(*Player))
            {
                UpdatePose();
                return;
            }
            if (Attack.bTransitionPending) return;
        }

        const FVector Offset = Player->GetActorLocation() - GetActorLocation();
        const double Distance = Offset.Size2D();
        if (Distance > ThrustRangeCm || FMath::Abs(Offset.Z) > MaxHeightCm || !HasSightOf(*Player))
            return;

        const Move Chosen = Distance > SweepRangeCm || (AttacksSinceLane % 2 != 0)
            ? Move::Thrust : Move::Sweep;
        if (Attack.Begin(Chosen, false, NextSweepSign))
        {
            AimAt(*Player);
            if (Chosen == Move::Sweep) NextSweepSign = -NextSweepSign;
            ++AttacksSinceLane;
            UpdatePose();
        }
        return;
    }

    if (Previous == Phase::Aim && PlayerAvailable)
        AimAt(*Player);

    if (Previous == Phase::Active)
    {
        if (Attack.Action == Move::LaneStep)
        {
            const FVector Offset = LaneDestination - GetActorLocation();
            const double Fraction = FMath::Min(1.0, DeltaSeconds / Attack.Remaining);
            FHitResult Blocker;
            AddActorWorldOffset(Offset * Fraction, true, &Blocker);
            if (Blocker.bBlockingHit)
            {
                Attack.StopLaneStep();
                UpdatePose();
                return;
            }
        }
        else if (PlayerAvailable && !Attack.bSpent)
        {
            const FVector Offset = Player->GetActorLocation() - GetActorLocation();
            if (Attack.TryHit(HasSightOf(*Player), FVector::DotProduct(Offset, GetActorForwardVector()),
                              FVector::DotProduct(Offset, GetActorRightVector()), Offset.Z, DeltaSeconds))
            {
                UGameplayStatics::ApplyDamage(Player, Attack.Action == Move::Sweep ? SweepDamage : ThrustDamage,
                                               nullptr, this, nullptr);
            }
        }
    }

    Attack.AdvanceTime(DeltaSeconds);
    UpdatePose();
}

void AEidolonRootclaw::UpdatePose()
{
    using namespace EidolonRootclaw;
    const FVector Base = GetActorLocation();
    const FVector Forward = GetActorForwardVector();
    const FVector Right = GetActorRightVector();
    const bool Preparing = Attack.Current == Phase::Aim || Attack.Current == Phase::Lock;
    const bool Active = Attack.Current == Phase::Active;
    const bool RecoveringSweep = Attack.Current == Phase::Recovery && Attack.Action == Move::Sweep;

    const double Angle = Attack.Action == Move::Sweep && (Preparing || Active) ? Attack.SweepAngle() : 24.0;
    const FVector ClawDirection = Forward.RotateAngleAxis(Angle, FVector::UpVector);
    HeavyClaw->SetWorldLocationAndRotation(Base + ClawDirection * 218.0 +
        FVector(0.0, 0.0, Preparing ? 95.0 : RecoveringSweep ? -80.0 : 18.0), ClawDirection.Rotation() + FRotator(-70.0, 0.0, 0.0));
    HeavyClaw->SetWorldScale3D(Attack.bSecondPhase ? FVector(1.55, 1.55, 2.85) : FVector(1.45, 1.45, 2.65));

    const double SmallReach = Attack.Action == Move::Thrust && Active ? 390.0 : 138.0;
    LightClaw->SetWorldLocationAndRotation(Base + Forward * SmallReach - Right * 97.0,
        GetActorRotation() + FRotator(-79.0, 0.0, 0.0));
    LightClaw->SetWorldScale3D(FVector(0.7, 0.7, 1.55));

    GroundMarker->SetHiddenInGame(!Preparing && !Active);
    if (!Preparing && !Active) return;

    if (Attack.Action == Move::LaneStep)
    {
        GroundMarker->SetWorldLocationAndRotation(LaneDestination - FVector(0.0, 0.0, GroundMarkerDropCm), GetActorRotation());
        GroundMarker->SetWorldScale3D(FVector(3.4, 2.8, 0.035));
        return;
    }

    const bool Sweep = Attack.Action == Move::Sweep;
    const FVector Direction = Sweep ? ClawDirection : Forward;
    const double Range = Sweep ? SweepRangeCm : ThrustRangeCm;
    GroundMarker->SetWorldLocationAndRotation(Base + Direction * (Range / 2.0) -
        FVector(0.0, 0.0, GroundMarkerDropCm), Direction.Rotation());
    GroundMarker->SetWorldScale3D(FVector(Range / 100.0, Sweep ? 1.25 : 2.0 * ThrustHalfWidthCm / 100.0,
        Active ? 0.075 : 0.03));
}

float AEidolonRootclaw::TakeDamage(float Amount, const FDamageEvent& Event,
                                   AController* Instigator, AActor* Causer)
{
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!HasAuthority() || !Player || Player->IsDefeated() || Causer != Player || !Instigator
        || Player->GetController() != Instigator || !FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return 0.0f;
    }

    const float Capped = FMath::Min(Amount, GetHealth());
    const float Applied = Super::TakeDamage(Capped * static_cast<float>(Attack.DamageMultiplier()),
                                            Event, Instigator, Causer);
    Attack.ObserveHealth(GetHealth(), StartingHealth);
    return Applied;
}
