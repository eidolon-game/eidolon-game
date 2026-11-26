#include "EidolonMineBorer.h"

#include "../EidolonCharacter.h"
#include "CollisionQueryParams.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr double RetreatSpeedCm = 230.0;
    constexpr float ChargeDamage = 25.0f;
    constexpr float DebrisDamage = 12.0f;
}

AEidolonMineBorer::AEidolonMineBorer()
{
    using namespace EidolonMineBorer;
    auto* VisualBody = CastChecked<UStaticMeshComponent>(GetRootComponent());
    CollisionBody = CreateDefaultSubobject<UBoxComponent>(TEXT("BorerCollision"));
    SetRootComponent(CollisionBody);
    CollisionBody->InitBoxExtent(FVector(BodyHalfLengthCm, BodyHalfWidthCm, BodyHalfHeightCm));
    CollisionBody->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    CollisionBody->SetGenerateOverlapEvents(false);
    VisualBody->SetupAttachment(CollisionBody);
    VisualBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualBody->SetRelativeScale3D(FVector(4.35, 3.80, 2.82));

    Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DiggingHead"));
    LeftPlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRearPlate"));
    RightPlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRearPlate"));
    DangerMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BorerGroundMarker"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Wedge(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plate(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (auto* Part : {Head.Get(), LeftPlate.Get(), RightPlate.Get(), DangerMarker.Get()})
    {
        Part->SetupAttachment(CollisionBody);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
    }
    if (Wedge.Succeeded()) Head->SetStaticMesh(Wedge.Object);
    if (Plate.Succeeded())
    {
        LeftPlate->SetStaticMesh(Plate.Object);
        RightPlate->SetStaticMesh(Plate.Object);
        DangerMarker->SetStaticMesh(Plate.Object);
    }
    DangerMarker->SetCastShadow(false);
    Head->SetRelativeScale3D(FVector(1.28, 1.52, 1.35));
    LeftPlate->SetRelativeScale3D(FVector(1.38, .76, .17));
    RightPlate->SetRelativeScale3D(FVector(1.38, .76, .17));
    UpdatePose();
}

bool AEidolonMineBorer::ConfigureChargeLane(const FVector& End)
{
    Home = GetActorLocation();
    const FVector Offset = End - Home;
    Direction = GetActorForwardVector().GetSafeNormal2D();
    LaneLength = FVector::DotProduct(Offset, Direction);
    const FVector Perpendicular = Offset - Direction * LaneLength;
    if (LaneLength < 600.0 || LaneLength > EidolonMineBorer::ChargeLimitCm + 1.0
        || Perpendicular.Size() > 2.0)
    {
        LaneLength = 0.0;
        return false;
    }
    Configure(TEXT("abyss_mine_borer"), 21);
    return true;
}

bool AEidolonMineBorer::CanSee(const AEidolonCharacter& Player) const
{
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EidolonBorerSight), false, this);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Player.GetActorLocation(),
                                                 ECC_Visibility, Params)
        || Hit.GetActor() == &Player;
}

void AEidolonMineBorer::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0 || LaneLength <= 0.0)
        return;

    using namespace EidolonMineBorer;
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const Phase Previous = Attack.Current;
    const double Progress = FVector::DotProduct(GetActorLocation() - Home, Direction);
    if (Previous == Phase::Ready)
    {
        if (Progress > .1)
        {
            AddActorWorldOffset(-Direction * FMath::Min(Progress, RetreatSpeedCm * DeltaSeconds), true);
            return;
        }
        if (!Player || Player->IsDefeated()) return;

        const FVector Offset = Player->GetActorLocation() - GetActorLocation();
        const double Ahead = FVector::DotProduct(Offset, Direction);
        const double Across = FVector::DotProduct(Offset, GetActorRightVector());
        if (Attack.Begin(CanSee(*Player), Ahead, Across, Offset.Z))
        {
            bHitPlayer = false;
            UpdatePose();
        }
        return;
    }

    if (Previous == Phase::Approach)
    {
        const double Step = ApproachSpeedCm * FMath::Min<double>(DeltaSeconds, Attack.Remaining);
        FHitResult Hit;
        AddActorWorldOffset(Direction * Step, true, &Hit);
        if (Hit.bBlockingHit) Attack.Advance(Attack.Remaining);
    }
    else if (Previous == Phase::Charge)
    {
        const double DistanceLeft = FMath::Max(0.0, LaneLength - Progress);
        const double Step = FMath::Min(DistanceLeft, Attack.ChargeDistance(DeltaSeconds));
        FHitResult Hit;
        if (Step > 0.0) AddActorWorldOffset(Direction * Step, true, &Hit);
        if (Hit.bBlockingHit)
        {
            if (Player && Hit.GetActor() == Player && !Hit.bStartPenetrating && !Player->IsDefeated())
            {
                bHitPlayer = true;
                UGameplayStatics::ApplyDamage(Player, ChargeDamage, nullptr, this, nullptr);
            }
            Attack.Stop();
        }
        else if (Step >= DistanceLeft)
        {
            Attack.Stop();
        }
    }

    if (Attack.Current == Previous) Attack.Advance(DeltaSeconds);
    if (Attack.Current == Previous) return;
    if (Attack.Current == Phase::Impact) ResolveDebris(Player);
    UpdatePose();
}

void AEidolonMineBorer::ResolveDebris(AEidolonCharacter* Player)
{
    if (bHitPlayer || !IsValid(Player) || Player->IsDefeated()) return;
    const FVector Offset = Player->GetActorLocation() - GetActorLocation();
    if (Attack.DebrisCanHit(FVector::DotProduct(Offset, Direction),
        FVector::DotProduct(Offset, GetActorRightVector()), Offset.Z) && CanSee(*Player))
    {
        bHitPlayer = true;
        UGameplayStatics::ApplyDamage(Player, DebrisDamage, nullptr, this, nullptr);
    }
}

void AEidolonMineBorer::UpdatePose()
{
    using namespace EidolonMineBorer;
    const bool bDigging = Attack.Current == Phase::Windup || Attack.Current == Phase::Charge;
    const bool bExposed = Attack.Current == Phase::Recovery;
    Head->SetRelativeLocation(FVector(173.0, 0.0, bDigging ? -75.0 : -30.0));
    Head->SetRelativeRotation(FRotator(bDigging ? -108.0 : -94.0, 0.0, 0.0));
    LeftPlate->SetRelativeLocation(FVector(-127.0, -91.0, bExposed ? 113.0 : 92.0));
    RightPlate->SetRelativeLocation(FVector(-127.0, 91.0, bExposed ? 113.0 : 92.0));
    LeftPlate->SetRelativeRotation(FRotator(-18.0, 0.0, bExposed ? -64.0 : -8.0));
    RightPlate->SetRelativeRotation(FRotator(-18.0, 0.0, bExposed ? 64.0 : 8.0));

    const bool bImpact = Attack.Current == Phase::Impact;
    DangerMarker->SetHiddenInGame(!(bDigging || bImpact));
    if (!(bDigging || bImpact)) return;

    if (bImpact)
    {
        DangerMarker->SetRelativeLocation(FVector(BodyHalfLengthCm + DebrisReachCm / 2.0, 0.0, -143.0));
        DangerMarker->SetRelativeScale3D(FVector(DebrisReachCm / 100.0, 2.0 * HazardHalfWidthCm / 100.0, .025));
    }
    else
    {
        const double DistanceLeft = FMath::Max(0.0, FVector::DotProduct(Home - GetActorLocation(), Direction) + LaneLength);
        const double Length = FMath::Max(20.0, DistanceLeft);
        DangerMarker->SetRelativeLocation(FVector(BodyHalfLengthCm + Length / 2.0, 0.0, -143.0));
        DangerMarker->SetRelativeScale3D(FVector(Length / 100.0, 2.0 * HazardHalfWidthCm / 100.0, .025));
    }
}

float AEidolonMineBorer::TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer)
{
    const auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!HasAuthority() || !Player || Player->IsDefeated() || Causer != Player || !Instigator
        || Player->GetController() != Instigator || !FMath::IsFinite(Amount) || Amount <= 0.0f)
        return 0.0f;

    const FVector Facing = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    const double Front = FVector::DotProduct(Facing, GetActorForwardVector().GetSafeNormal2D());
    return Super::TakeDamage(Amount * static_cast<float>(Attack.DamageMultiplier(Front)), Event, Instigator, Causer);
}
