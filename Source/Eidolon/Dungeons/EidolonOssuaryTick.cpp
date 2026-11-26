#include "EidolonOssuaryTick.h"

#include "../EidolonCharacter.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    const FVector ReadyScale(0.72, 0.78, 0.50);
    const FVector CrouchScale(0.80, 0.76, 0.33);
    constexpr float LeapDamage = 15.0f;
}

AEidolonOssuaryTick::AEidolonOssuaryTick()
{
    SetActorScale3D(ReadyScale);
}

void AEidolonOssuaryTick::BeginPlay()
{
    Super::BeginPlay();
    SetActorScale3D(ReadyScale);
    Configure(TEXT("ossuary_tick"), 2);
}

void AEidolonOssuaryTick::Tick(float DeltaSeconds)
{
    // Use the shared arena damage receiver and target representation with a distinct attack.
    AActor::Tick(DeltaSeconds);
    using namespace EidolonOssuaryTick;

    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!HasAuthority() || !Player || Player->IsDefeated()) return;

    if (Attack.Current == Phase::Ready)
    {
        const FVector Offset = Player->GetActorLocation() - GetActorLocation();
        const FVector Direction = Offset.GetSafeNormal2D();
        if (Direction.IsNearlyZero() || FMath::Abs(Offset.Z) >= 140.0) return;

        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EidolonOssuaryTick), false, this);
        FHitResult Sight;
        const bool Visible = !GetWorld()->LineTraceSingleByChannel(
            Sight, GetActorLocation(), Player->GetActorLocation(), ECC_Visibility, QueryParams)
            || Sight.GetActor() == Player;
        if (Attack.Begin(Visible, Offset.SizeSquared2D()))
        {
            LockedDirection = Direction;
            SetActorRotation(Direction.Rotation());
            SetActorScale3D(CrouchScale);
        }
        return;
    }

    if (Attack.Current == Phase::Leap)
    {
        const double Distance = Attack.LeapDistance(DeltaSeconds);
        if (Distance <= 0.0) return;

        FHitResult Hit;
        AddActorWorldOffset(LockedDirection * Distance, true, &Hit);
        if (Hit.bBlockingHit)
        {
            const bool HitPlayer = Hit.GetActor() == Player;
            if (Attack.Land(HitPlayer) && HitPlayer)
                UGameplayStatics::ApplyDamage(Player, LeapDamage, nullptr, this, nullptr);
            return;
        }
    }

    const auto Previous = Attack.Current;
    Attack.AdvanceTime(DeltaSeconds);
    if (Previous == Phase::Windup && Attack.Current == Phase::Leap)
        SetActorScale3D(ReadyScale);
}

float AEidolonOssuaryTick::TakeDamage(float Amount, const FDamageEvent& Event,
                                     AController* Instigator, AActor* Causer)
{
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!HasAuthority() || !Player || Player->IsDefeated() || Causer != Player || !Instigator ||
        Player->GetController() != Instigator || !FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return 0.0f;
    }

    return Super::TakeDamage(Amount, Event, Instigator, Causer);
}

void AEidolonOssuaryTick::OnLethalDamage()
{
    OnDefeated.Broadcast(*this);
}
