#include "EidolonForestStalker.h"

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
    constexpr double SightRangeCm = 1050.0;
    constexpr double HomeRadiusCm = 1120.0;
    constexpr double StalkSpeedCm = 190.0;
    constexpr double SearchSpeedCm = 145.0;
    constexpr double GroundStepCm = 35.0;
    constexpr double TelegraphDropCm = 91.0;
    constexpr float PounceDamage = 23.0f;
    constexpr float SweepDamage = 16.0f;
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
    if (FVector::DistSquared2D(Home, Position) > FMath::Square(HomeRadiusCm)) return false;

    const FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonForestStalkerGround), false, this);
    const FCollisionObjectQueryParams Surface(ECC_WorldStatic);
    FHitResult Hit;
    return GetWorld()->LineTraceSingleByObjectType(Hit, Position,
        Position - FVector(0.0, 0.0, 125.0), Surface, Query)
        && Hit.ImpactNormal.Z > 0.5;
}

bool AEidolonForestStalker::SweepGrounded(const FVector& Movement, FHitResult& Hit)
{
    const int32 Pieces = FMath::Max(1, FMath::CeilToInt(Movement.Size2D() / GroundStepCm));
    const FVector Step = Movement / static_cast<double>(Pieces);
    for (int32 Index = 0; Index < Pieces; ++Index)
    {
        if (!CanStandAt(GetActorLocation() + Step)) return false;

        AddActorWorldOffset(Step, true, &Hit);
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
    SweepGrounded(Direction * Distance, Hit);
}

void AEidolonForestStalker::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;

    using namespace EidolonForestStalker;
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    const bool PlayerAvailable = Player && !Player->IsDefeated();
    const bool Visible = PlayerAvailable && CanSee(*Player);
    Search.Observe(Visible, DeltaSeconds);
    if (Visible) LastSeen = Player->GetActorLocation();

    const Phase Previous = Attack.Current;
    if (Previous == Phase::Ready)
    {
        const FVector Offset = LastSeen - GetActorLocation();
        if (Visible && FMath::Abs(Offset.Z) <= MaxHeightDeltaCm && Attack.Begin(true, Offset.Size2D()))
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
