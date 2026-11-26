#include "EidolonMarrowRam.h"

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
    constexpr float ChargeDamage = 25.0f;
    constexpr double MaxLockHeightCm = 125.0;
    constexpr double RetreatSpeedCm = 210.0;
}

AEidolonMarrowRam::AEidolonMarrowRam()
{
    SetActorScale3D(FVector(1.46, 1.14, 0.88));
    HeadPlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadPlate"));
    LeftHoof = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftHoof"));
    RightHoof = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightHoof"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeadMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HoofMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (UStaticMeshComponent* Part : {HeadPlate.Get(), LeftHoof.Get(), RightHoof.Get()})
    {
        Part->SetupAttachment(GetRootComponent());
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
    }
    if (HeadMesh.Succeeded()) HeadPlate->SetStaticMesh(HeadMesh.Object);
    if (HoofMesh.Succeeded())
    {
        LeftHoof->SetStaticMesh(HoofMesh.Object);
        RightHoof->SetStaticMesh(HoofMesh.Object);
    }
    HeadPlate->SetRelativeLocation(FVector(49.0, 0.0, 12.0));
    HeadPlate->SetRelativeScale3D(FVector(0.63, 0.84, 0.70));
    LeftHoof->SetRelativeScale3D(FVector(0.30, 0.24, 0.43));
    RightHoof->SetRelativeScale3D(FVector(0.30, 0.24, 0.43));
    UpdatePose();
}

void AEidolonMarrowRam::BeginPlay()
{
    Super::BeginPlay();
    Home = GetActorLocation();
    Configure(TEXT("abyss_marrow_ram"), 5);
}

void AEidolonMarrowRam::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);
    if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;

    using namespace EidolonMarrowRam;
    const Phase Previous = Attack.Current;
    if (Previous == Phase::Ready)
    {
        const FVector ToHome = Home - GetActorLocation();
        const double HomeDistance = ToHome.Size();
        if (HomeDistance > 6.0)
        {
            // Return along the lane by sweep; don't count retreat contact as a charge.
            const double Step = FMath::Min(HomeDistance, RetreatSpeedCm * DeltaSeconds);
            AddActorWorldOffset(ToHome.GetSafeNormal() * Step, true);
            return;
        }

        auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
        if (!Player || Player->IsDefeated()) return;
        const FVector Offset = Player->GetActorLocation() - GetActorLocation();
        if (FMath::Abs(Offset.Z) > MaxLockHeightCm) return;

        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EidolonMarrowRam), false, this);
        FHitResult Sight;
        const bool Visible = !GetWorld()->LineTraceSingleByChannel(
            Sight, GetActorLocation(), Player->GetActorLocation(), ECC_Visibility, QueryParams)
            || Sight.GetActor() == Player;
        if (Attack.Begin(Visible, Offset.Size2D()))
        {
            LockedDirection = Offset.GetSafeNormal2D();
            SetActorRotation(LockedDirection.Rotation());
            UpdatePose();
        }
        return;
    }

    if (Previous == Phase::Charge)
    {
        FHitResult Hit;
        AddActorWorldOffset(LockedDirection * Attack.ChargeDistance(DeltaSeconds), true, &Hit);
        if (Hit.bBlockingHit)
        {
            AActor* Blocker = Hit.GetActor();
            auto* Player = Cast<AEidolonCharacter>(Blocker);
            if (Player)
            {
                Attack.Stop(Impact::Player);
                if (!Hit.bStartPenetrating && !Player->IsDefeated())
                    UGameplayStatics::ApplyDamage(Player, ChargeDamage, nullptr, this, nullptr);
            }
            else
            {
                Attack.Stop(Impact::Obstacle);
                if (Blocker && !Hit.bStartPenetrating && !Blocker->IsActorBeingDestroyed())
                    OnChargeObstructed.Broadcast(*this, *Blocker);
            }
            UpdatePose();
            return;
        }
    }

    Attack.AdvanceTime(DeltaSeconds);
    if (Attack.Current != Previous) UpdatePose();
}

void AEidolonMarrowRam::UpdatePose()
{
    using namespace EidolonMarrowRam;
    LeftHoof->SetRelativeLocation(FVector(25.0, -36.0, Attack.Current == Phase::FirstScrape ? -14.0 : -32.0));
    RightHoof->SetRelativeLocation(FVector(25.0, 36.0, Attack.Current == Phase::SecondScrape ? -14.0 : -32.0));

    switch (Attack.Current)
    {
        case Phase::FirstScrape: HeadPlate->SetRelativeRotation(FRotator(-44.0, 0.0, -9.0)); break;
        case Phase::SecondScrape: HeadPlate->SetRelativeRotation(FRotator(-63.0, 0.0, 9.0)); break;
        case Phase::Charge: HeadPlate->SetRelativeRotation(FRotator(-87.0, 0.0, 0.0)); break;
        case Phase::Recovery:
            HeadPlate->SetRelativeRotation(Attack.LastImpact == Impact::Obstacle
                ? FRotator(-29.0, 27.0, 42.0) : FRotator(-49.0, 0.0, 0.0));
            break;
        case Phase::Ready: HeadPlate->SetRelativeRotation(FRotator(-62.0, 0.0, 0.0)); break;
    }
}

float AEidolonMarrowRam::TakeDamage(float Amount, const FDamageEvent& Event,
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
