#include "EidolonForestStalker.h"

#include "../EidolonCharacter.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr double SightRangeCm = 1050.0;
    constexpr double HomeRadiusCm = 1120.0;
    constexpr double StalkSpeedCm = 190.0;
    constexpr double SearchSpeedCm = 145.0;
    constexpr double GroundStepCm = 35.0;
    constexpr double TelegraphDropCm = 91.0;
    constexpr float PounceDamage = 23.0f;
    constexpr float SweepDamage = 16.0f;
    constexpr float PerchDamage = 26.0f;
    constexpr double PerchAnchorToleranceCm = 22.0;
    constexpr double PerchPlayerRangeCm = 390.0;
}

AEidolonForestStalker::AEidolonForestStalker()
{
    SetActorScale3D(FVector(2.05, 1.32, 1.7));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FName Name(*FString::Printf(TEXT("RootHook%d"), Index));
        UStaticMeshComponent* Hook = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Hook->SetupAttachment(GetRootComponent());
        Hook->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Hook->SetGenerateOverlapEvents(false);
        Hook->SetRelativeScale3D(FVector(0.28, 0.3, 1.2));
        if (Cone.Succeeded()) Hook->SetStaticMesh(Cone.Object);
        Hooks.Add(Hook);
    }

    Crest = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarkCrest"));
    Crest->SetupAttachment(GetRootComponent());
    Crest->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Crest->SetGenerateOverlapEvents(false);
    Crest->SetRelativeLocation(FVector(-14.0, 0.0, 45.0));
    Crest->SetRelativeScale3D(FVector(0.75, 0.6, 0.75));
    if (Cone.Succeeded()) Crest->SetStaticMesh(Cone.Object);

    Telegraph = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Telegraph"));
    Telegraph->SetupAttachment(GetRootComponent());
    Telegraph->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Telegraph->SetGenerateOverlapEvents(false);
    Telegraph->SetCastShadow(false);
    Telegraph->SetHiddenInGame(true);
    if (Cube.Succeeded()) Telegraph->SetStaticMesh(Cube.Object);

    PerchMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PerchLandingMarker"));
    PerchMarker->SetupAttachment(GetRootComponent());
    PerchMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PerchMarker->SetGenerateOverlapEvents(false);
    PerchMarker->SetCastShadow(false);
    PerchMarker->SetHiddenInGame(true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded()) PerchMarker->SetStaticMesh(Cylinder.Object);
}

void AEidolonForestStalker::BeginPlay()
{
    Super::BeginPlay();
    Home = GetActorLocation();
    Configure(TEXT("abyss_forest_stalker"), 11);
    UpdatePose();
}

bool AEidolonForestStalker::CanSee(const AEidolonCharacter& Player) const
{
    const FVector Target = Player.GetActorLocation();
    if (FVector::DistSquared2D(Home, Target) > FMath::Square(HomeRadiusCm)
        || FVector::DistSquared2D(GetActorLocation(), Target) > FMath::Square(SightRangeCm))
    {
        return false;
    }
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonForestStalkerSight), false, this);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Target, ECC_Visibility, Query)
        || Hit.GetActor() == &Player;
}

bool AEidolonForestStalker::CanStandAt(const FVector& Position) const
{
    double SupportZ = 0.0;
    return ReadSupportZ(Position, 125.0, SupportZ);
}

void AEidolonForestStalker::EnableCanalGroundFollowing()
{
    if (HasAuthority()) bCanalGroundFollowing = true;
}

void AEidolonForestStalker::SetAttackPartner(AEidolonForestStalker* Other)
{
    if (HasAuthority()) AttackPartner = Other != this ? Other : nullptr;
}

bool AEidolonForestStalker::IsAttackInProgress() const
{
    return GetHealth() > 0.0f && !IsActorBeingDestroyed()
        && (Attack.Current != EidolonForestStalker::Phase::Ready
            || Perch == PerchStage::Telegraph
            || (Perch == PerchStage::Drop && !Flight.bAborted));
}

bool AEidolonForestStalker::IsPartnerAttacking() const
{
    const AEidolonForestStalker* Partner = AttackPartner.Get();
    return Partner && Partner->IsAttackInProgress();
}

bool AEidolonForestStalker::ReadSupportZ(const FVector& Position, double Depth, double& SupportZ) const
{
    if (FVector::DistSquared2D(Home, Position) > FMath::Square(HomeRadiusCm)) return false;

    const FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonForestStalkerGround), false, this);
    const FCollisionObjectQueryParams Surface(ECC_WorldStatic);
    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByObjectType(Hit, Position,
        Position - FVector(0.0, 0.0, Depth), Surface, Query) || Hit.ImpactNormal.Z <= 0.5)
    {
        return false;
    }
    SupportZ = Hit.ImpactPoint.Z;
    return true;
}

bool AEidolonForestStalker::ResolveGroundStep(FVector& Position, bool bFollowGrade) const
{
    if (!bCanalGroundFollowing) return CanStandAt(Position);

    using namespace EidolonForestStalker;
    // The extra centimetre lets an exact legal drop hit before the trace end.
    constexpr double ProbeDepth = PerchCenterHeightCm + CanalMaxGroundStepCm + 1.0;
    double SupportZ = 0.0;
    if (!ReadSupportZ(Position, ProbeDepth, SupportZ)) return false;

    const auto RootZ = CanalRootZ(Position.Z, SupportZ, bFollowGrade);
    if (!RootZ) return false;
    Position.Z = *RootZ;
    return true;
}

bool AEidolonForestStalker::CanStartGroundAttack(const FVector& Offset) const
{
    using namespace EidolonForestStalker;
    if (!bCanalGroundFollowing) return FMath::Abs(Offset.Z) <= MaxHeightDeltaCm;
    if (FMath::Abs(Offset.Z) > CanalMaxGroundStepCm) return false;
    const double Distance = Offset.Size2D();
    if (Distance <= SweepRangeCm || Distance > PounceRangeCm) return true;

    const FVector Start = GetActorLocation();
    const FVector Horizontal(Offset.X, Offset.Y, 0.0);
    const int32 Pieces = FMath::CeilToInt(Distance / GroundStepCm);
    for (int32 Index = 1; Index <= Pieces; ++Index)
    {
        FVector Sample = Start + Horizontal * (static_cast<double>(Index) / Pieces);
        if (!ResolveGroundStep(Sample, false)) return false;
    }
    return true;
}

bool AEidolonForestStalker::SweepGrounded(const FVector& Movement, FHitResult& Hit, bool bFollowGrade)
{
    const int32 Pieces = FMath::Max(1, FMath::CeilToInt(Movement.Size2D() / GroundStepCm));
    const FVector Step = Movement / static_cast<double>(Pieces);
    for (int32 Index = 0; Index < Pieces; ++Index)
    {
        const FVector Current = GetActorLocation();
        FVector Destination = Current + Step;
        if (!ResolveGroundStep(Destination, bFollowGrade)) return false;

        AddActorWorldOffset(Destination - Current, true, &Hit);
        if (Hit.bBlockingHit) return false;
    }
    return true;
}

void AEidolonForestStalker::StalkToward(const FVector& Destination, double DeltaSeconds, double Speed)
{
    const FVector Offset = Destination - GetActorLocation();
    const FVector Direction = Offset.GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    const double Distance = FMath::Min(Offset.Size2D(), FMath::Min(GroundStepCm, Speed * DeltaSeconds));
    SetActorRotation(Direction.Rotation());
    FHitResult Hit;
    SweepGrounded(Direction * Distance, Hit, true);
}

bool AEidolonForestStalker::ConfigurePerch(const FVector& PerchFeet, const FVector& LandingFeet,
                                          bool bWaitForPlayer)
{
    using namespace EidolonForestStalker;
    if (!HasAuthority() || Perch != PerchStage::Disabled || Attack.Current != Phase::Ready ||
        PerchFeet.ContainsNaN() || LandingFeet.ContainsNaN() ||
        !IsPerchRouteValid(FVector::Dist2D(PerchFeet, LandingFeet), PerchFeet.Z - LandingFeet.Z))
    {
        return false;
    }

    const FVector Offset(0.0, 0.0, PerchCenterHeightCm);
    const FVector Upper = PerchFeet + Offset;
    const FVector Landing = LandingFeet + Offset;
    const bool AlreadyUpper = FVector::DistSquared(GetActorLocation(), Upper) <= FMath::Square(PerchAnchorToleranceCm);
    const FVector Lower = AlreadyUpper ? Landing : GetActorLocation();
    if (!CanStandAt(Upper) || !CanStandAt(Landing) || !CanStandAt(Lower) ||
        !IsPerchRouteValid(FVector::Dist2D(Upper, Lower), Upper.Z - Lower.Z))
    {
        return false;
    }

    PerchCenter = Upper;
    PerchGround = Lower;
    LandingCenter = Landing;
    PerchRemaining = PerchWaitSeconds;
    bAwaitInitialPerchTarget = bWaitForPlayer && AlreadyUpper && IsPerchPathClear(LandingCenter);
    Perch = AlreadyUpper ? PerchStage::Waiting : PerchStage::Ground;
    return true;
}

FVector AEidolonForestStalker::GetPerchRoutePoint(const FVector& Lower, double Alpha) const
{
    FVector Point = FMath::Lerp(PerchCenter, Lower, Alpha);
    Point.Z = EidolonForestStalker::PerchAltitude(PerchCenter.Z, Lower.Z, Alpha);
    return Point;
}

bool AEidolonForestStalker::IsPerchPathClear(const FVector& Lower, const AActor* PreviewTarget) const
{
    const FQuat Facing = (Lower - PerchCenter).GetSafeNormal2D().Rotation().Quaternion();
    const FCollisionShape Body = FCollisionShape::MakeBox(FVector(103.0, 68.0, 85.0));
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestStalkerPerch), false, this);
    if (PreviewTarget) Query.AddIgnoredActor(PreviewTarget);

    if (GetWorld()->OverlapBlockingTestByChannel(PerchCenter, Facing, ECC_Pawn, Body, Query)
        || GetWorld()->OverlapBlockingTestByChannel(Lower, Facing, ECC_Pawn, Body, Query))
    {
        return false;
    }

    constexpr int32 Segments = 16;
    FVector Previous = PerchCenter;
    for (int32 Index = 1; Index <= Segments; ++Index)
    {
        const FVector Next = GetPerchRoutePoint(Lower, static_cast<double>(Index) / Segments);
        if (GetWorld()->SweepTestByChannel(Previous, Next, Facing, ECC_Pawn, Body, Query))
            return false;
        Previous = Next;
    }
    return true;
}

void AEidolonForestStalker::BeginPerchTravel(PerchStage Stage, const FVector& Lower, bool bAscending)
{
    Perch = Stage;
    FlightLower = Lower;
    Flight = {bAscending ? 1.0 : 0.0, bAscending, false};
    const FVector Direction = (Lower - PerchCenter).GetSafeNormal2D();
    SetActorRotation((bAscending ? -Direction : Direction).Rotation());
}

void AEidolonForestStalker::ShowPerchLanding(bool bShow)
{
    using namespace EidolonForestStalker;
    PerchMarker->SetHiddenInGame(!bShow);
    Crest->SetRelativeRotation(FRotator(bShow ? 55.0 : 12.0, 0.0, 0.0));
    if (!bShow) return;

    const FVector Surface(0.0, 0.0, PerchCenterHeightCm - 3.0);
    PerchMarker->SetWorldLocation(LandingCenter - Surface);
    const double DiameterScale = 2.0 * PerchLandingRadiusCm / 100.0;
    PerchMarker->SetWorldScale3D(FVector(DiameterScale, DiameterScale, 0.035));
}

void AEidolonForestStalker::TickPerchTravel(AEidolonCharacter* Player, double DeltaSeconds)
{
    using namespace EidolonForestStalker;
    const double NextAlpha = Flight.Next(DeltaSeconds);
    FHitResult Hit;
    if (NextAlpha != Flight.Alpha)
    {
        const FVector Destination = GetPerchRoutePoint(FlightLower, NextAlpha);
        AddActorWorldOffset(Destination - GetActorLocation(), true, &Hit);
        if (Hit.bBlockingHit)
        {
            // FHitResult::Time keeps the cursor close to the actual swept
            // position, including partial movement before the obstruction.
            Flight.Alpha = FMath::Lerp(Flight.Alpha, NextAlpha, static_cast<double>(Hit.Time));
            Flight.Obstructed();
            ShowPerchLanding(false);
            return;
        }
        Flight.Alpha = NextAlpha;
    }
    if (!Flight.Arrived()) return;

    ShowPerchLanding(false);
    if (Flight.bTowardUpper)
    {
        Perch = PerchStage::Waiting;
        PerchRemaining = Flight.bAborted ? 0.70 : PerchWaitSeconds;
        return;
    }

    const bool Landed = Perch == PerchStage::Drop && !Flight.bAborted
        && FVector::DistSquared(GetActorLocation(), LandingCenter) <= FMath::Square(PerchAnchorToleranceCm);
    Perch = PerchStage::Disabled;
    Home = FlightLower;
    if (Landed)
    {
        const FVector Offset = Player ? Player->GetActorLocation() - LandingCenter : FVector::ZeroVector;
        const bool Caught = Player && !Player->IsDefeated() && IsPerchLandingHit(
            true, CanSee(*Player), Offset.SizeSquared2D(), Offset.Z);
        if (Caught) UGameplayStatics::ApplyDamage(Player, PerchDamage, nullptr, this, nullptr);
        Attack.Current = Phase::Recovery;
        Attack.bHit = Caught;
        Attack.Remaining = Caught ? HitRecoverySeconds : MissRecoverySeconds;
    }
    UpdatePose();
}

bool AEidolonForestStalker::TickPerch(AEidolonCharacter* Player, bool bVisible, double DeltaSeconds)
{
    using namespace EidolonForestStalker;
    switch (Perch)
    {
        case PerchStage::Disabled:
            return false;
        case PerchStage::Ground:
            if (Attack.Current != Phase::Ready || !bVisible ||
                FVector::DistSquared(GetActorLocation(), PerchGround) > FMath::Square(PerchAnchorToleranceCm)
                || !IsPerchPathClear(PerchGround))
            {
                return false;
            }
            BeginPerchTravel(PerchStage::Climb, PerchGround, true);
            return true;
        case PerchStage::Waiting:
            if (bAwaitInitialPerchTarget)
            {
                if (!bVisible) return true;
                bAwaitInitialPerchTarget = false;
            }
            if (bVisible && FVector::DistSquared2D(Player->GetActorLocation(), LandingCenter) <= FMath::Square(PerchPlayerRangeCm))
            {
                if (IsPartnerAttacking()) return true;
                if (IsPerchPathClear(LandingCenter, Player))
                {
                    Perch = PerchStage::Telegraph;
                    PerchRemaining = PerchWindupSeconds;
                    SetActorRotation((LandingCenter - PerchCenter).GetSafeNormal2D().Rotation());
                    ShowPerchLanding(true);
                    return true;
                }
            }
            PerchRemaining = FMath::Max(0.0, PerchRemaining - DeltaSeconds);
            if (PerchRemaining <= 0.0 && IsPerchPathClear(PerchGround))
                BeginPerchTravel(PerchStage::ReturnToGround, PerchGround, false);
            return true;
        case PerchStage::Telegraph:
            PerchRemaining = FMath::Max(0.0, PerchRemaining - DeltaSeconds);
            if (PerchRemaining <= 0.0)
            {
                if (IsPerchPathClear(LandingCenter))
                    BeginPerchTravel(PerchStage::Drop, LandingCenter, false);
                else
                {
                    ShowPerchLanding(false);
                    if (IsPerchPathClear(PerchGround))
                    {
                        BeginPerchTravel(PerchStage::ReturnToGround, PerchGround, false);
                    }
                    else
                    {
                        Perch = PerchStage::Waiting;
                        PerchRemaining = 0.70;
                    }
                }
            }
            return true;
        case PerchStage::Climb:
        case PerchStage::Drop:
        case PerchStage::ReturnToGround:
            TickPerchTravel(Player, DeltaSeconds);
            return true;
    }

    return false;
}

void AEidolonForestStalker::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;

    using namespace EidolonForestStalker;
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const bool PlayerAvailable = Player && !Player->IsDefeated();
    const bool Visible = PlayerAvailable && CanSee(*Player);
    if (TickPerch(Player, Visible, DeltaSeconds)) return;
    Search.Observe(Visible, DeltaSeconds);
    if (Visible) LastSeen = Player->GetActorLocation();

    const Phase Previous = Attack.Current;
    if (Previous == Phase::Ready)
    {
        const FVector Offset = LastSeen - GetActorLocation();
        if (Visible && !IsPartnerAttacking() && CanStartGroundAttack(Offset)
            && Attack.Begin(true, Offset.Size2D()))
        {
            LockedDirection = Offset.GetSafeNormal2D();
            LockedPlayer = Player;
            SetActorRotation(LockedDirection.Rotation());
            UpdatePose();
            return;
        }

        if (Search.Current == Awareness::Returning)
        {
            StalkToward(Home, DeltaSeconds, SearchSpeedCm);
        }
        else if (Offset.SizeSquared2D() > FMath::Square(38.0))
        {
            StalkToward(LastSeen, DeltaSeconds, Visible ? StalkSpeedCm : SearchSpeedCm);
        }
        else if (!Visible)
        {
            // At the last seen point, visibly scan instead of following through walls.
            FRotator Facing = GetActorRotation();
            Facing.Yaw += 42.0 * DeltaSeconds;
            SetActorRotation(Facing);
        }
        return;
    }

    if (Previous == Phase::Active)
    {
        if (Attack.Action == Move::Pounce)
        {
            FHitResult Hit;
            if (!SweepGrounded(LockedDirection * Attack.PounceDistance(DeltaSeconds), Hit))
            {
                const bool HitLockedPlayer = PlayerAvailable && LockedPlayer.Get() == Player
                    && Hit.bBlockingHit && !Hit.bStartPenetrating && Hit.GetActor() == Player;
                Attack.StopPounce(HitLockedPlayer);
                if (HitLockedPlayer) UGameplayStatics::ApplyDamage(Player, PounceDamage, nullptr, this, nullptr);
                UpdatePose();
                return;
            }
        }
        else if (Visible)
        {
            const FVector Offset = Player->GetActorLocation() - GetActorLocation();
            if (Attack.TrySweepHit(true, FVector::DotProduct(Offset, GetActorForwardVector()),
                                 FVector::DotProduct(Offset, GetActorRightVector()), Offset.Z))
            {
                UGameplayStatics::ApplyDamage(Player, SweepDamage, nullptr, this, nullptr);
            }
        }
    }

    Attack.AdvanceTime(DeltaSeconds);
    if (Attack.Current != Previous) UpdatePose();
}

void AEidolonForestStalker::UpdatePose()
{
    using namespace EidolonForestStalker;
    const bool Windup = Attack.Current == Phase::Windup;
    const bool Active = Attack.Current == Phase::Active;
    const bool Pounce = Attack.Action == Move::Pounce;
    const bool Exposed = Attack.Current == Phase::Recovery && !Attack.bHit;
    for (int32 Index = 0; Index < Hooks.Num(); ++Index)
    {
        const bool Front = Index < 2;
        const double Side = Index % 2 == 0 ? -1.0 : 1.0;
        Hooks[Index]->SetRelativeLocation(FVector(Front ? 31.0 : -39.0, Side * 43.0, Windup ? -8.0 : -22.0));
        Hooks[Index]->SetRelativeRotation(FRotator(Front && Active ? -65.0 : Windup ? 34.0 : 14.0, 0.0,
            Side * (Exposed ? 68.0 : Windup && !Pounce ? 52.0 : 29.0)));
    }
    Crest->SetRelativeRotation(FRotator(Windup ? 50.0 : Exposed ? -25.0 : 12.0, 0.0, 0.0));

    // After release, the pounce's marker is hidden so it doesn't track the moving body.
    Telegraph->SetHiddenInGame(!Windup && !(Active && !Pounce));
    if (!Windup && !(Active && !Pounce)) return;
    const double Length = Pounce ? Attack.LockedDistanceCm : SweepRangeCm;
    Telegraph->SetWorldLocationAndRotation(GetActorLocation() + GetActorForwardVector() * (Length / 2.0)
        - FVector(0.0, 0.0, TelegraphDropCm), GetActorRotation());
    Telegraph->SetWorldScale3D(FVector(Length / 100.0, Pounce ? 1.35 : 2.85, Active ? 0.07 : 0.028));
}

float AEidolonForestStalker::TakeDamage(float Amount, const FDamageEvent& Event,
                                        AController* Instigator, AActor* Causer)
{
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!HasAuthority() || !Player || Player->IsDefeated() || Causer != Player || !Instigator
        || Player->GetController() != Instigator || !FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return 0.0f;
    }

    const float CappedAmount = FMath::Min(Amount, GetHealth());
    return Super::TakeDamage(CappedAmount * static_cast<float>(Attack.DamageMultiplier()),
                             Event, Instigator, Causer);
}
