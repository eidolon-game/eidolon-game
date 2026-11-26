#include "EidolonVaultPincer.h"

#include "../EidolonCharacter.h"
#include "CollisionQueryParams.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr float SweepDamage = 20.0f;
    constexpr double SweepMarkerDropCm = 45.0;
}

AEidolonVaultPincer::AEidolonVaultPincer()
{
    SetActorScale3D(FVector(1.2, 1.0, 0.85));

    Claw = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Claw"));
    Claw->SetupAttachment(GetRootComponent());
    Claw->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Claw->SetRelativeScale3D(FVector(0.72, 1.15, 0.22));
    Claw->SetRelativeLocation(FVector(62.0, 0.0, 5.0));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Shape.Succeeded()) Claw->SetStaticMesh(Shape.Object);

    LeftMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftMarker"));
    CenterMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CenterMarker"));
    RightMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightMarker"));
    for (UStaticMeshComponent* Marker : {LeftMarker.Get(), CenterMarker.Get(), RightMarker.Get()})
    {
        Marker->SetupAttachment(GetRootComponent());
        Marker->SetAbsolute(true, true, true);
        Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Marker->SetCastShadow(false);
        Marker->SetHiddenInGame(true);
        if (Shape.Succeeded()) Marker->SetStaticMesh(Shape.Object);
    }

    PhaseLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PhaseLight"));
    PhaseLight->SetupAttachment(GetRootComponent());
    PhaseLight->SetRelativeLocation(FVector(50.0, 0.0, 45.0));
    PhaseLight->SetAttenuationRadius(250.0f);
    PhaseLight->SetCastShadows(false);
    PhaseLight->SetVisibility(false);
}

void AEidolonVaultPincer::Tick(float DeltaSeconds)
{
    // Use the arena damage receiver and defeated delegate with a fixed encounter pose.
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority()) return;

    using namespace EidolonVaultPincer;
    const Phase Previous = Attack.Current;
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const bool bPlayerAvailable = Player && !Player->IsDefeated();

    if (Previous == Phase::Guard)
    {
        if (!bPlayerAvailable) return;
        const FVector Offset = Player->GetActorLocation() - GetActorLocation();
        const double Distance = Offset.Size2D();
        if (Distance <= 0.0 || Distance > ReachCm || FMath::Abs(Offset.Z) > HeightCm)
            return;

        if (Attack.Begin(Distance, Offset.Z, HasClearSweep(*Player)))
        {
            SetActorRotation(Offset.GetSafeNormal2D().Rotation());
            UpdatePresentation();
        }
        return;
    }

    const bool bStrike = Attack.Advance(DeltaSeconds);
    if (Attack.Current != Previous) UpdatePresentation();
    if (bStrike && bPlayerAvailable)
    {
        const FVector Offset = Player->GetActorLocation() - GetActorLocation();
        const double Dot = FVector::DotProduct(GetActorForwardVector(), Offset.GetSafeNormal2D());
        if (Attack.Hits(Offset.Size2D(), Offset.Z, Dot, HasClearSweep(*Player)))
            UGameplayStatics::ApplyDamage(Player, SweepDamage, nullptr, this, nullptr);
    }
}

bool AEidolonVaultPincer::HasClearSweep(const AActor& Target) const
{
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EidolonVaultPincerSweep), false, this);
    Params.AddIgnoredActor(&Target);
    return !GetWorld()->LineTraceTestByChannel(GetActorLocation(), Target.GetActorLocation(), ECC_Visibility, Params);
}

float AEidolonVaultPincer::TakeDamage(float Amount, const FDamageEvent& Event,
                                     AController* Instigator, AActor* Causer)
{
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!HasAuthority() || !Player || Player->IsDefeated() || Causer != Player || !Instigator
        || Player->GetController() != Instigator || !FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return 0.0f;
    }

    const FVector Direction = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    const double Dot = FVector::DotProduct(GetActorForwardVector(), Direction);
    return Super::TakeDamage(Amount * Attack.DamageMultiplier(Dot), Event, Instigator, Causer);
}

void AEidolonVaultPincer::UpdatePresentation()
{
    using namespace EidolonVaultPincer;
    const bool bTelegraph = Attack.Current == Phase::Telegraph;
    const bool bSweep = Attack.Current == Phase::Sweep;
    const bool bRecovery = Attack.Current == Phase::Recovery;
    const bool bShowMarkers = bTelegraph || bSweep;
    const FVector Floor = GetActorLocation() - FVector(0.0, 0.0, SweepMarkerDropCm);

    auto PlaceMarker = [&](UStaticMeshComponent* Marker, double YawOffset)
    {
        Marker->SetHiddenInGame(!bShowMarkers);
        if (!bShowMarkers) return;

        const FRotator Heading(0.0, GetActorRotation().Yaw + YawOffset, 0.0);
        Marker->SetWorldLocationAndRotation(Floor + Heading.Vector() * (ReachCm * 0.5), Heading);
        Marker->SetWorldScale3D(FVector(ReachCm / 100.0, 0.08, bSweep ? 0.07 : 0.025));
    };

    PlaceMarker(LeftMarker, -ArcHalfAngleDegrees);
    PlaceMarker(CenterMarker, 0.0);
    PlaceMarker(RightMarker, ArcHalfAngleDegrees);

    PhaseLight->SetVisibility(Attack.Current != Phase::Guard);
    if (bRecovery)
    {
        Claw->SetRelativeRotation(FRotator(45.0, 75.0, 0.0));
        PhaseLight->SetLightColor(FLinearColor(0.16f, 0.64f, 1.0f));
        PhaseLight->SetIntensity(1800.0f);
    }
    else
    {
        const double Yaw = bTelegraph ? -55.0 : (bSweep ? 55.0 : 0.0);
        Claw->SetRelativeRotation(FRotator(0.0, Yaw, 0.0));
        PhaseLight->SetLightColor(FLinearColor(1.0f, bSweep ? 0.19f : 0.63f, 0.05f));
        PhaseLight->SetIntensity(bSweep ? 4000.0f : 2100.0f);
    }
}
