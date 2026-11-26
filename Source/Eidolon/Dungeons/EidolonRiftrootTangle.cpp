#include "EidolonRiftrootTangle.h"

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
    const FVector BodyScale(4.10, 3.50, 3.10);
    constexpr double CenterHeightCm = 165.0;
    constexpr double SpawnToleranceCm = 40.0;
    constexpr double MinimumAdvanceCm = 300.0;
    constexpr double MaximumAdvanceCm = 850.0;
    constexpr double AwarenessRangeCm = 970.0;
    constexpr double CenterSupportToleranceCm = 20.0;
    constexpr double TravelSupportStepCm = 45.0;
    constexpr float WedgeDamage = 34.0f;
    constexpr float RootDamage = 26.0f;
}

AEidolonRiftrootTangle::AEidolonRiftrootTangle()
{
    SetActorScale3D(BodyScale);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Boulder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Boulder"));
    Cavity = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cavity"));
    GroundMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMarker"));
    for (UStaticMeshComponent* Part : {Boulder.Get(), Cavity.Get(), GroundMarker.Get()})
    {
        Part->SetupAttachment(GetRootComponent());
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        if (Cube.Succeeded()) Part->SetStaticMesh(Cube.Object);
    }
    GroundMarker->SetCastShadow(false);
    GroundMarker->SetHiddenInGame(true);
    Cavity->SetHiddenInGame(true);

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FName Name(*FString::Printf(TEXT("GroundRoot%d"), Index));
        UStaticMeshComponent* Root = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Root->SetupAttachment(GetRootComponent());
        Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Root->SetGenerateOverlapEvents(false);
        if (Cone.Succeeded()) Root->SetStaticMesh(Cone.Object);
        Roots.Add(Root);
    }
}

void AEidolonRiftrootTangle::BeginPlay()
{
    Super::BeginPlay();
    Configure(TEXT("boss"), 20);
    SetActorScale3D(BodyScale);
    StartingHealth = GetHealth();
    UpdatePose();
}

bool AEidolonRiftrootTangle::ConfigureFaultAdvance(const FVector& SpawnFeet, const FVector& DestinationFeet)
{
    using namespace EidolonRiftroot;
    if (!HasAuthority() || Attack.Current != Phase::Ready || Attack.bAdvanceConsumed
        || SpawnFeet.ContainsNaN() || DestinationFeet.ContainsNaN())
    {
        return false;
    }

    const FVector Offset(0.0, 0.0, CenterHeightCm);
    const FVector Origin = SpawnFeet + Offset;
    const FVector Destination = DestinationFeet + Offset;
    const FVector Travel = Destination - Origin;
    const double Length = Travel.Size2D();
    if (FVector::DistSquared(Origin, GetActorLocation()) > FMath::Square(SpawnToleranceCm)
        || Length < MinimumAdvanceCm || Length > MaximumAdvanceCm
        || FMath::Abs(Travel.Z) > CenterSupportToleranceCm
        || !HasGroundSupport(Origin) || !HasGroundSupport(Destination))
    {
        return false;
    }
    FaultDestination = Destination;
    bFaultConfigured = true;
    return true;
}

bool AEidolonRiftrootTangle::HasSightOf(const AEidolonCharacter& Player) const
{
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonRiftrootSight), false, this);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Player.GetActorLocation(), ECC_Visibility, Query)
        || Hit.GetActor() == &Player;
}

bool AEidolonRiftrootTangle::HasGroundSupport(const FVector& Center) const
{
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonRiftrootGround), false, this);
    const FCollisionObjectQueryParams Floor(ECC_WorldStatic);
    FHitResult Hit;
    return GetWorld()->LineTraceSingleByObjectType(Hit, Center,
        Center - FVector(0.0, 0.0, CenterHeightCm + CenterSupportToleranceCm + 1.0), Floor, Query)
        && Hit.ImpactNormal.Z > 0.75
        && FMath::Abs(Center.Z - Hit.ImpactPoint.Z - CenterHeightCm) <= CenterSupportToleranceCm;
}

bool AEidolonRiftrootTangle::IsFaultRouteClear() const
{
    if (!bFaultConfigured) return false;
    const FVector Start = GetActorLocation();
    const FVector Travel = FaultDestination - Start;
    const FQuat Facing = Travel.GetSafeNormal2D().Rotation().Quaternion();
    const FCollisionShape Body = FCollisionShape::MakeBox(FVector(205.0, 175.0, 155.0));
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonRiftrootTravel), false, this);
    if (GetWorld()->OverlapBlockingTestByChannel(FaultDestination, Facing, ECC_Pawn, Body, Query)
        || GetWorld()->SweepTestByChannel(Start, FaultDestination, Facing, ECC_Pawn, Body, Query))
    {
        return false;
    }

    const int32 Pieces = FMath::Max(1, FMath::CeilToInt(Travel.Size2D() / TravelSupportStepCm));
    for (int32 Index = 1; Index <= Pieces; ++Index)
    {
        if (!HasGroundSupport(Start + Travel * (static_cast<double>(Index) / Pieces))) return false;
    }
    return true;
}

void AEidolonRiftrootTangle::AimAt(const FVector& Target)
{
    const FVector Direction = (Target - GetActorLocation()).GetSafeNormal2D();
    if (!Direction.IsNearlyZero()) SetActorRotation(Direction.Rotation());
}

void AEidolonRiftrootTangle::ClipRockWedge(const AEidolonCharacter* Player)
{
    using namespace EidolonRiftroot;
    const FVector Origin = GetActorLocation() - FVector(0.0, 0.0, CenterHeightCm - 25.0);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonRiftrootWedge), false, this);
    if (Player) Query.AddIgnoredActor(Player);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Origin,
        Origin + GetActorForwardVector() * WedgeEndCm, ECC_Visibility, Query))
    {
        Attack.ClipWedgeAt(Hit.Distance);
    }
}

void AEidolonRiftrootTangle::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;

    using namespace EidolonRiftroot;
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const bool PlayerAvailable = Player && !Player->IsDefeated();
    const Phase Previous = Attack.Current;
    if (Previous == Phase::Ready)
    {
        if (!PlayerAvailable) return;
        if (Attack.bAdvancePending)
        {
            if (Attack.Begin(Move::FaultAdvance))
            {
                if (bFaultConfigured) AimAt(FaultDestination);
                UpdatePose();
            }
            return;
        }

        const FVector Offset = Player->GetActorLocation() - GetActorLocation();
        if (Offset.SizeSquared2D() > FMath::Square(AwarenessRangeCm)
            || FMath::Abs(Offset.Z) > HitHeightCm || !HasSightOf(*Player))
        {
            return;
        }

        if (bNextRootDraw)
        {
            const double Side = FVector::DotProduct(Offset, GetActorRightVector());
            LastRootSide = FMath::Abs(Side) < RootInnerCm ? -LastRootSide : Side > 0.0 ? 1 : -1;
            if (Attack.Begin(Move::RootDraw, LastRootSide)) bNextRootDraw = false;
        }
        else if (Attack.Begin(Move::RockWedge))
        {
            bNextRootDraw = true;
            AimAt(Player->GetActorLocation());
        }
        UpdatePose();
        return;
    }

    if (Previous == Phase::Aim && PlayerAvailable && HasSightOf(*Player))
        AimAt(Player->GetActorLocation());

    if (Previous == Phase::Active)
    {
        if (Attack.Kind == Move::FaultAdvance)
        {
            if (bFaultMoving)
            {
                const FVector Offset = FaultDestination - GetActorLocation();
                const FVector Movement = Offset * FMath::Min(1.0, DeltaSeconds / Attack.Remaining);
                const int32 Pieces = FMath::Max(1, FMath::CeilToInt(Movement.Size() / TravelSupportStepCm));
                const FVector Step = Movement / static_cast<double>(Pieces);
                for (int32 Index = 0; Index < Pieces; ++Index)
                {
                    FHitResult Hit;
                    const bool bSupported = HasGroundSupport(GetActorLocation() + Step);
                    if (bSupported) AddActorWorldOffset(Step, true, &Hit);
                    if (!bSupported || Hit.bBlockingHit)
                    {
                        bFaultMoving = false;
                        Attack.StopAdvance();
                        UpdatePose();
                        return;
                    }
                }
            }
        }
        else if (PlayerAvailable && !Attack.bHitSpent)
        {
            const FVector Offset = Player->GetActorLocation() - GetActorLocation();
            if (Attack.TryHit(HasSightOf(*Player), FVector::DotProduct(Offset, GetActorForwardVector()),
                FVector::DotProduct(Offset, GetActorRightVector()), Offset.Z))
            {
                UGameplayStatics::ApplyDamage(Player, Attack.Kind == Move::RockWedge ? WedgeDamage : RootDamage,
                                               nullptr, this, nullptr);
            }
        }
    }

    Attack.AdvanceTime(DeltaSeconds);
    if (Previous == Phase::Aim && Attack.Current == Phase::Lock) ClipRockWedge(Player);
    if (Previous == Phase::Lock && Attack.Current == Phase::Active && Attack.Kind == Move::FaultAdvance)
        bFaultMoving = IsFaultRouteClear();
    UpdatePose();
}

void AEidolonRiftrootTangle::UpdatePose()
{
    using namespace EidolonRiftroot;
    const FVector Base = GetActorLocation();
    const FVector Front = GetActorForwardVector();
    const FVector Right = GetActorRightVector();
    const FRotator Rotation = GetActorRotation();
    const bool Telegraph = Attack.Current == Phase::Aim || Attack.Current == Phase::Lock;
    const bool Active = Attack.Current == Phase::Active;
    const bool Recovering = Attack.Current == Phase::Recovery;
    const bool Wedge = Attack.Kind == Move::RockWedge;
    const bool Fault = Attack.Kind == Move::FaultAdvance;

    Boulder->SetWorldLocationAndRotation(Base + Front * (Wedge && Active ? 230.0 : 142.0)
        + FVector(0.0, 0.0, Telegraph && Wedge ? 117.0 : Recovering && Wedge ? -95.0 : 28.0),
        Rotation + FRotator(Telegraph && Wedge ? -33.0 : 14.0, 11.0, -13.0));
    Boulder->SetWorldScale3D(FVector(1.65, 1.95, 1.10));

    Cavity->SetHiddenInGame(!Recovering || Attack.Kind == Move::FaultAdvance);
    const double CavitySide = Wedge ? -1.0 : Attack.RootSide;
    Cavity->SetWorldLocationAndRotation(Base + Right * (176.0 * CavitySide) - Front * 64.0, Rotation);
    Cavity->SetWorldScale3D(FVector(0.74, 0.48, 1.06));

    for (int32 Index = 0; Index < Roots.Num(); ++Index)
    {
        const double Side = Index % 2 == 0 ? -1.0 : 1.0;
        const bool Drawn = Attack.Kind == Move::RootDraw && (Telegraph || Active) && Side == Attack.RootSide;
        const double Along = Index < 2 ? 112.0 : -137.0;
        const double ShiftProgress = Fault && Telegraph ? 1.0 - Attack.Remaining / GetTiming(Move::FaultAdvance).Telegraph : 0.0;
        const bool FootRaised = Fault && Telegraph && FMath::FloorToInt(ShiftProgress * Roots.Num()) == Index;
        Roots[Index]->SetWorldLocationAndRotation(Base + Front * Along + Right * Side * (Drawn ? 245.0 : 143.0)
            - FVector(0.0, 0.0, Drawn ? 28.0 : FootRaised ? 49.0 : 81.0),
            Rotation + FRotator(-9.0, 0.0, Side * (Drawn ? 72.0 : 28.0)));
        Roots[Index]->SetWorldScale3D(FVector(0.68, 0.72, Drawn ? 2.35 : 1.65));
    }

    const bool MarkFault = Fault && ((Telegraph && bFaultConfigured) || (Active && bFaultMoving));
    const bool MarkAttack = !Fault && (Telegraph || Active) && (!Wedge || Attack.LockedWedgeEndCm > WedgeStartCm);
    const bool Mark = MarkFault || MarkAttack;
    GroundMarker->SetHiddenInGame(!Mark);
    if (!Mark) return;

    FVector Ground = Base - FVector(0.0, 0.0, CenterHeightCm - 3.0);
    FVector Dimensions;
    if (Wedge)
    {
        Ground += Front * ((WedgeStartCm + Attack.LockedWedgeEndCm) / 2.0);
        Dimensions = FVector(Attack.LockedWedgeEndCm - WedgeStartCm, WedgeHalfWidthCm * 2.0, Active ? 7.0 : 3.0);
    }
    else if (Attack.Kind == Move::RootDraw)
    {
        Ground += Front * ((RootFrontCm + RootBackCm) / 2.0)
            + Right * (Attack.RootSide * (RootOuterCm + RootInnerCm) / 2.0);
        Dimensions = FVector(RootFrontCm - RootBackCm, RootOuterCm - RootInnerCm, Active ? 7.0 : 3.0);
    }
    else
    {
        Ground = FaultDestination - FVector(0.0, 0.0, CenterHeightCm - 3.0);
        Dimensions = FVector(410.0, 350.0, 3.0);
    }
    GroundMarker->SetWorldLocationAndRotation(Ground, Rotation);
    GroundMarker->SetWorldScale3D(Dimensions / 100.0);
}

float AEidolonRiftrootTangle::TakeDamage(float Amount, const FDamageEvent& Event,
                                         AController* Instigator, AActor* Causer)
{
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!HasAuthority() || !Player || Player->IsDefeated() || Causer != Player || !Instigator
        || Player->GetController() != Instigator || !FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return 0.0f;
    }

    const FVector Offset = Player->GetActorLocation() - GetActorLocation();
    const double Multiplier = Attack.DamageMultiplier(FVector::DotProduct(Offset, GetActorForwardVector()),
                                                       FVector::DotProduct(Offset, GetActorRightVector()));
    const float Capped = FMath::Min(Amount, GetHealth());
    const float Applied = Super::TakeDamage(Capped * static_cast<float>(Multiplier), Event, Instigator, Causer);
    Attack.ObserveHealth(GetHealth(), StartingHealth);
    return Applied;
}
