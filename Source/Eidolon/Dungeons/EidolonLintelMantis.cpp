#include "EidolonLintelMantis.h"

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
    constexpr float DiveDamage = 21.0f;
    constexpr double MaxLockHeightCm = 220.0;
    constexpr double BodyClearanceCm = 45.0;
    constexpr double TravelToClearLedgeCm = 165.0;
}

AEidolonLintelMantis::AEidolonLintelMantis()
{
    SetActorScale3D(FVector(1.15, 0.93, 0.74));
    LeftBlade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftBlade"));
    RightBlade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightBlade"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BladeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    for (UStaticMeshComponent* Blade : {LeftBlade.Get(), RightBlade.Get()})
    {
        Blade->SetupAttachment(GetRootComponent());
        Blade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Blade->SetGenerateOverlapEvents(false);
        Blade->SetRelativeScale3D(FVector(0.23, 0.23, 1.05));
        if (BladeMesh.Succeeded()) Blade->SetStaticMesh(BladeMesh.Object);
    }
    LeftBlade->SetRelativeLocation(FVector(35.0, -46.0, 13.0));
    RightBlade->SetRelativeLocation(FVector(35.0, 46.0, 13.0));
    LandingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LandingMarker"));
    LandingMarker->SetupAttachment(GetRootComponent());
    LandingMarker->SetAbsolute(true, true, true);
    LandingMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LandingMarker->SetGenerateOverlapEvents(false);
    LandingMarker->SetCastShadow(false);
    LandingMarker->SetWorldScale3D(FVector(1.9, 1.9, 0.035));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (MarkerMesh.Succeeded()) LandingMarker->SetStaticMesh(MarkerMesh.Object);
    UpdateTelegraph();
}

void AEidolonLintelMantis::BeginPlay()
{
    Super::BeginPlay();
    SetActorScale3D(FVector(1.15, 0.93, 0.74));
    PerchForward = GetActorForwardVector().GetSafeNormal2D();
    Configure(TEXT("abyss_lintel_mantis"), 3);
}

void AEidolonLintelMantis::Tick(float DeltaSeconds)
{
    // The Mantis uses shared damage reception without the Arena attack routine.
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority()) return;

    using EidolonLintelMantis::Phase;
    const Phase Previous = Attack.Current;
    if (Previous == Phase::Ready)
    {
        auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
        if (!Player || Player->IsDefeated()) return;

        const FVector Offset = Player->GetActorLocation() - GetActorLocation();
        if (FMath::Abs(Offset.Z) > MaxLockHeightCm) return;

        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EidolonLintelMantis), false, this);
        FHitResult Sight;
        const bool Visible = !GetWorld()->LineTraceSingleByChannel(
            Sight, GetActorLocation() + FVector(0, 0, 50), Player->GetActorLocation(), ECC_Visibility, QueryParams)
            || Sight.GetActor() == Player;
        FHitResult Ground;
        const FVector Target = Player->GetActorLocation();
        QueryParams.AddIgnoredActor(Player);
        const bool HasGround = Visible && GetWorld()->LineTraceSingleByChannel(
            Ground, Target, Target - FVector(0, 0, 250), ECC_Visibility, QueryParams);
        if (Attack.Begin(HasGround, Offset.Size2D()))
        {
            LockedDirection = Offset.GetSafeNormal2D();
            LockedPlayer = Player;
            DiveTravel = 0.0;
            DropHeight = FMath::Max(0.0, GetActorLocation().Z - Ground.ImpactPoint.Z - BodyClearanceCm);
            LedgeClearance = DropHeight > 60.0
                ? TravelToClearLedgeCm / FMath::Max(0.55, FVector::DotProduct(LockedDirection, PerchForward)) : 0.0;
            LandingMarker->SetWorldLocation(Ground.ImpactPoint + FVector(0, 0, 3));
            SetActorRotation(LockedDirection.Rotation());
            UpdateTelegraph();
        }
        return;
    }

    if (Previous == Phase::Dive)
    {
        const double Distance = Attack.DiveDistance(DeltaSeconds);
        if (Distance <= 0.0) return;

        const double ToLedge = FMath::Clamp(LedgeClearance - DiveTravel, 0.0, Distance);
        if (ToLedge > 0.0 && SweepDive(LockedDirection * ToLedge)) return;
        const double NextTravel = DiveTravel + Distance;
        const double Drop = DropHeight * (EidolonLintelMantis::DropProgress(NextTravel, LedgeClearance, Attack.LockedDistanceCm)
            - EidolonLintelMantis::DropProgress(DiveTravel, LedgeClearance, Attack.LockedDistanceCm));
        if (Distance > ToLedge && SweepDive(LockedDirection * (Distance - ToLedge) - FVector(0, 0, Drop))) return;
        DiveTravel = NextTravel;
    }

    Attack.AdvanceTime(DeltaSeconds);
    if (Attack.Current != Previous) UpdateTelegraph();
}

bool AEidolonLintelMantis::SweepDive(const FVector& Movement)
{
    FHitResult Impact;
    AddActorWorldOffset(Movement, true, &Impact);
    if (!Impact.bBlockingHit) return false;

    auto* Player = LockedPlayer.Get();
    const bool HitPlayer = Player && !Player->IsDefeated() && Impact.GetActor() == Player;
    Attack.Land(HitPlayer);
    if (HitPlayer) UGameplayStatics::ApplyDamage(Player, DiveDamage, nullptr, this, nullptr);
    UpdateTelegraph();
    return true;
}

void AEidolonLintelMantis::UpdateTelegraph()
{
    using EidolonLintelMantis::Phase;
    LandingMarker->SetVisibility(Attack.Current == Phase::Windup);
    switch (Attack.Current)
    {
        case Phase::Windup:
            LeftBlade->SetRelativeRotation(FRotator(0.0, -38.0, -64.0));
            RightBlade->SetRelativeRotation(FRotator(0.0, 38.0, 64.0));
            break;
        case Phase::Dive:
            LeftBlade->SetRelativeRotation(FRotator(-78.0, -8.0, 0.0));
            RightBlade->SetRelativeRotation(FRotator(-78.0, 8.0, 0.0));
            break;
        case Phase::MissRecovery:
            LeftBlade->SetRelativeRotation(FRotator(-18.0, -62.0, 89.0));
            RightBlade->SetRelativeRotation(FRotator(-18.0, 62.0, -89.0));
            break;
        case Phase::Ready:
        case Phase::HitRecovery:
            LeftBlade->SetRelativeRotation(FRotator(-32.0, -20.0, -24.0));
            RightBlade->SetRelativeRotation(FRotator(-32.0, 20.0, 24.0));
            break;
    }
}

float AEidolonLintelMantis::TakeDamage(float Amount, const FDamageEvent& Event,
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
