#include "EidolonCenserHagfish.h"

#include "../EidolonCharacter.h"
#include "CollisionQueryParams.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr double StripDropCm = 28.0;
    constexpr float StripDamage = 16.0f;
}

AEidolonCenserHagfish::AEidolonCenserHagfish()
{
    SetActorScale3D(FVector(1.35, 0.65, 0.52));

    Strip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Strip"));
    Strip->SetupAttachment(GetRootComponent());
    Strip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Strip->SetGenerateOverlapEvents(false);
    Strip->SetCastShadow(false);
    Strip->SetHiddenInGame(true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Shape.Succeeded()) Strip->SetStaticMesh(Shape.Object);

    StripLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StripLight"));
    StripLight->SetupAttachment(GetRootComponent());
    StripLight->SetAttenuationRadius(420.0f);
    StripLight->SetCastShadows(false);
    StripLight->SetVisibility(false);
}

void AEidolonCenserHagfish::BeginPlay()
{
    Super::BeginPlay();
    Configure(TEXT("abyss_censer_hagfish"), 4);
}

bool AEidolonCenserHagfish::HasAvailableWorldClaim() const
{
    // Including telegraphs prevents another moored enemy from locking a strip
    // before this cast activates. A destroyed actor takes its strip with it.
    for (TActorIterator<AEidolonCenserHagfish> It(GetWorld()); It; ++It)
    {
        if (*It != this && !It->IsActorBeingDestroyed() && It->Attack.HoldsWorldClaim())
            return false;
    }
    return true;
}

void AEidolonCenserHagfish::Tick(float DeltaSeconds)
{
    // Independent fixed-position attack; do not run the Arena melee AI.
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority()) return;

    using namespace EidolonCenserHagfish;
    const Phase Previous = Attack.Current;
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const bool PlayerAvailable = Player && !Player->IsDefeated();

    if (Previous == Phase::Ready)
    {
        if (!PlayerAvailable || !HasAvailableWorldClaim()) return;

        const FVector Origin = GetActorLocation() - FVector(0.0, 0.0, StripDropCm);
        const FVector Offset = Player->GetActorLocation() - Origin;
        const double Distance = Offset.Size2D();
        const FVector Direction = Offset.GetSafeNormal2D();
        if (Direction.IsNearlyZero() || Distance > MaxLockRangeCm || FMath::Abs(Offset.Z) > HeightToleranceCm)
            return;

        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EidolonCenserHagfish), false, this);
        QueryParams.AddIgnoredActor(Player);
        FHitResult Bank;
        const bool Blocked = GetWorld()->LineTraceSingleByChannel(
            Bank, Origin, Origin + Direction * MaxStripLengthCm, ECC_Visibility, QueryParams);
        const double UnobstructedCm = Blocked ? Bank.Distance : MaxStripLengthCm;
        if (Attack.Begin(true, Distance, UnobstructedCm))
        {
            LockedOrigin = Origin;
            LockedDirection = Direction;
            SetActorRotation(Direction.Rotation());
            UpdateStrip();
        }
        return;
    }

    if (Previous == Phase::Active && PlayerAvailable)
    {
        const FVector Offset = Player->GetActorLocation() - LockedOrigin;
        const FVector SideDirection(-LockedDirection.Y, LockedDirection.X, 0.0);
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EidolonCenserHagfishImpact), false, this);
        QueryParams.AddIgnoredActor(Player);
        const FVector SurfacePoint = LockedOrigin + FVector(Offset.X, Offset.Y, 0.0);
        if (!GetWorld()->LineTraceTestByChannel(LockedOrigin, SurfacePoint, ECC_Visibility, QueryParams)
            && Attack.TryHit(FVector::DotProduct(Offset, LockedDirection),
                             FVector::DotProduct(Offset, SideDirection), Offset.Z))
        {
            UGameplayStatics::ApplyDamage(Player, StripDamage, nullptr, this, nullptr);
        }
    }

    Attack.AdvanceTime(DeltaSeconds);
    if (Attack.Current != Previous) UpdateStrip();
}

void AEidolonCenserHagfish::UpdateStrip()
{
    const bool Visible = Attack.HoldsWorldClaim();
    Strip->SetHiddenInGame(!Visible);
    StripLight->SetVisibility(Visible);
    if (!Visible) return;

    const bool Hot = Attack.Current == EidolonCenserHagfish::Phase::Active;
    const double Height = Hot ? 6.0 : 2.0;
    const FVector Center = LockedOrigin + LockedDirection * (Attack.LengthCm / 2.0);
    Strip->SetWorldLocationAndRotation(Center + FVector(0.0, 0.0, Height / 2.0), LockedDirection.Rotation());
    Strip->SetWorldScale3D(FVector(Attack.LengthCm, EidolonCenserHagfish::HalfWidthCm * 2.0, Height) / 100.0);
    StripLight->SetWorldLocation(Center + FVector(0.0, 0.0, 42.0));
    StripLight->SetLightColor(Hot ? FLinearColor(1.0f, 0.20f, 0.05f) : FLinearColor(1.0f, 0.72f, 0.28f));
    StripLight->SetIntensity(Hot ? 4600.0f : 1600.0f);
}

float AEidolonCenserHagfish::TakeDamage(float Amount, const FDamageEvent& Event,
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
