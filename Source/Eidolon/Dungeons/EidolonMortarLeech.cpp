#include "EidolonMortarLeech.h"

#include "../EidolonCharacter.h"
#include "CollisionQueryParams.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AEidolonMortarLeech::AEidolonMortarLeech()
{
    SetActorScale3D(FVector(1.05, 0.65, 0.80));

    Lance = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lance"));
    Lance->SetupAttachment(GetRootComponent());
    Lance->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Lance->SetCastShadow(false);
    Lance->SetHiddenInGame(true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Shape.Succeeded()) Lance->SetStaticMesh(Shape.Object);

    TelegraphLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TelegraphLight"));
    TelegraphLight->SetupAttachment(GetRootComponent());
    TelegraphLight->SetAttenuationRadius(350.0f);
    TelegraphLight->SetCastShadows(false);
    TelegraphLight->SetVisibility(false);
}

void AEidolonMortarLeech::Tick(float DeltaSeconds)
{
    // Keep the arena damage receiver but bypass the arena movement and attacks.
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority()) return;

    using namespace EidolonMortarLeech;
    const Phase Previous = Attack.Current;
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const bool bPlayerAvailable = Player && !Player->IsDefeated();

    if (Previous == Phase::Ready)
    {
        if (!bPlayerAvailable) return;

        const FVector Origin = GetActorLocation();
        const FVector Offset = Player->GetActorLocation() - Origin;
        const FVector Direction = Offset.GetSafeNormal2D();
        if (Direction.IsNearlyZero() || Offset.SizeSquared2D() > FMath::Square(MaxReachCm)
            || FMath::Abs(Offset.Z) > HeightCm)
        {
            return;
        }

        if (Attack.Begin(Offset.Size2D(), Offset.Z, ClearLength(Origin, Direction, MaxReachCm, Player)))
        {
            LockedDirection = Direction;
            SetActorRotation(Direction.Rotation());
            UpdateLance();
        }
        return;
    }

    if (Attack.Advance(DeltaSeconds) && bPlayerAvailable) ResolveStrike(*Player);
    if (Attack.Current != Previous) UpdateLance();
}

double AEidolonMortarLeech::ClearLength(const FVector& Origin, const FVector& Direction,
                                       double MaxLength, const AEidolonCharacter* Player) const
{
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EidolonMortarLeechCover), false, this);
    Params.AddIgnoredActor(Player);
    FHitResult Cover;
    if (GetWorld()->LineTraceSingleByChannel(Cover, Origin, Origin + Direction * MaxLength, ECC_Visibility, Params))
    {
        return Cover.Distance;
    }
    return MaxLength;
}

void AEidolonMortarLeech::ResolveStrike(AEidolonCharacter& Player)
{
    const FVector Origin = GetActorLocation();
    Attack.LengthCm = ClearLength(Origin, LockedDirection, Attack.LengthCm, &Player);
    const FVector Offset = Player.GetActorLocation() - Origin;
    const FVector Perpendicular(-LockedDirection.Y, LockedDirection.X, 0.0);
    if (!Attack.Contains(FVector::DotProduct(Offset, LockedDirection),
                         FVector::DotProduct(Offset, Perpendicular), Offset.Z))
    {
        return;
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(EidolonMortarLeechStrike), false, this);
    Params.AddIgnoredActor(&Player);
    if (!GetWorld()->LineTraceTestByChannel(Origin, Player.GetActorLocation(), ECC_Visibility, Params))
    {
        constexpr float LanceDamage = 18.0f;
        UGameplayStatics::ApplyDamage(&Player, LanceDamage, nullptr, this, nullptr);
    }
}

void AEidolonMortarLeech::UpdateLance()
{
    using namespace EidolonMortarLeech;
    const bool bStrike = Attack.Current == Phase::Strike;
    const bool bVisible = Attack.LengthCm > 0.0 && (Attack.Current == Phase::Telegraph || bStrike);
    Lance->SetHiddenInGame(!bVisible);
    TelegraphLight->SetVisibility(bVisible);
    if (!bVisible) return;

    const FVector Center = GetActorLocation() + LockedDirection * (Attack.LengthCm * 0.5);
    Lance->SetWorldLocationAndRotation(Center, LockedDirection.Rotation());
    Lance->SetWorldScale3D(FVector(Attack.LengthCm, HalfWidthCm * 2.0, bStrike ? 12.0 : 3.0) / 100.0);
    TelegraphLight->SetWorldLocation(Center + FVector(0, 0, 35));
    TelegraphLight->SetLightColor(bStrike ? FLinearColor(1.0f, 0.22f, 0.10f) : FLinearColor(0.85f, 0.67f, 0.30f));
    TelegraphLight->SetIntensity(bStrike ? 4000.0f : 1500.0f);
}
