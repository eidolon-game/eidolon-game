#include "EidolonMineBorer.h"

#include "../EidolonCharacter.h"
#include "EidolonMineBorerPresentation.h"
#include "CollisionQueryParams.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
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
    VisualBody = CastChecked<UStaticMeshComponent>(GetRootComponent());
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
    ActionAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("BorerActionAudio"));
    ChargeAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("BorerChargeAudio"));
    for (auto* Audio : {ActionAudio.Get(), ChargeAudio.Get()})
    {
        Audio->SetupAttachment(CollisionBody);
        Audio->SetAutoActivate(false);
        Audio->bOverrideAttenuation = true;
        Audio->AttenuationOverrides.bSpatialize = true;
        Audio->AttenuationOverrides.bAttenuate = true;
        Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(250.0, 0.0, 0.0);
        Audio->AttenuationOverrides.FalloffDistance = 1450.0f;
        Audio->AttenuationOverrides.bEnableOcclusion = true;
        Audio->AttenuationOverrides.OcclusionLowPassFilterFrequency = 1800.0f;
        Audio->AttenuationOverrides.OcclusionVolumeAttenuation = 0.40f;
    }
    ActionAudio->SetRelativeLocation(FVector(BodyHalfLengthCm + 16.0, 0.0, -30.0));
    ChargeAudio->SetRelativeLocation(FVector(BodyHalfLengthCm + 8.0, 0.0, -121.0));
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
    ConfigurePresentation();
    return true;
}

void AEidolonMineBorer::ConfigurePresentation()
{
    const auto* Asset = LoadObject<UEidolonMineBorerPresentation>(nullptr,
        TEXT("/Game/Eidolon/Creatures/MineBorer/DA_MineBorerPresentation.DA_MineBorerPresentation"));
    if (Asset)
    {
        WindupSound = Asset->WindupSound;
        ChargeSound = Asset->ChargeSound;
        ImpactSound = Asset->ImpactSound;
    }
    if (!Asset || !Asset->Body || !Asset->Head || !Asset->RearLeft || !Asset->RearRight)
    {
        UE_LOG(LogTemp, Warning, TEXT("Mine Borer presentation is unavailable. Using native graybox meshes."));
        return;
    }

    const int32 Axes[] = {Asset->ImportedAxes.X, Asset->ImportedAxes.Y, Asset->ImportedAxes.Z};
    uint8 Seen = 0;
    for (const int32 Axis : Axes)
    {
        if (Axis == 0 || Axis < -3 || Axis > 3) return;
        const uint8 Bit = static_cast<uint8>(1u << (FMath::Abs(Axis) - 1));
        if (Seen & Bit) return;
        Seen |= Bit;
    }
    if (!FMath::IsFinite(Asset->ImportScale) || Asset->ImportScale <= 0.0
        || Asset->RearHingeAxis.ContainsNaN() || !Asset->RearHingeAxis.IsNormalized()
        || !FMath::IsFinite(Asset->RearOpenRadians) || Asset->RearOpenRadians <= 0.0)
        return;

    const auto CanonicalVector = [&Axes, Asset](const FVector& Value)
    {
        FVector Result;
        for (int32 Index = 0; Index < 3; ++Index)
            Result[Index] = Value[FMath::Abs(Axes[Index]) - 1] * (Axes[Index] < 0 ? -1.0 : 1.0);
        return Result * Asset->ImportScale;
    };
    const FTransform Alignment{FMatrix(
        FPlane(CanonicalVector(FVector::ForwardVector), 0.0),
        FPlane(CanonicalVector(FVector::RightVector), 0.0),
        FPlane(CanonicalVector(FVector::UpVector), 0.0),
        FPlane(0.0, 0.0, 0.0, 1.0))};
    PresentationAlignment = Alignment.GetRotation();
    RearHingeAxis = Asset->RearHingeAxis;
    RearOpenRadians = Asset->RearOpenRadians;
    const auto Apply = [&Alignment](UStaticMeshComponent& Component, UStaticMesh* Mesh, const FVector& Pivot)
    {
        Component.SetStaticMesh(Mesh);
        Component.SetRelativeTransform(FTransform(Alignment.GetRotation(), Pivot, Alignment.GetScale3D()));
    };
    Apply(*VisualBody.Get(), Asset->Body.Get(), Asset->BodyPivot);
    Apply(*Head.Get(), Asset->Head.Get(), Asset->HeadPivot);
    Apply(*LeftPlate.Get(), Asset->RearLeft.Get(), Asset->RearLeftPivot);
    Apply(*RightPlate.Get(), Asset->RearRight.Get(), Asset->RearRightPivot);
    bHasPresentation = true;
    UpdatePose();
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
    PlayPhaseSound();
}

void AEidolonMineBorer::PlayPhaseSound()
{
    using EidolonMineBorer::Phase;
    switch (Attack.Current)
    {
        case Phase::Windup:
            if (WindupSound)
            {
                ActionAudio->SetSound(WindupSound.Get());
                ActionAudio->Play();
            }
            break;
        case Phase::Charge:
            if (ChargeSound)
            {
                ChargeAudio->SetSound(ChargeSound.Get());
                ChargeAudio->Play();
            }
            break;
        case Phase::Impact:
            if (ChargeAudio->IsPlaying()) ChargeAudio->FadeOut(0.07f, 0.0f);
            if (ImpactSound)
            {
                ActionAudio->SetSound(ImpactSound.Get());
                ActionAudio->Play();
            }
            break;
        case Phase::Ready:
        case Phase::Approach:
        case Phase::Recovery:
            break;
    }
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
    if (bHasPresentation)
    {
        const FQuat Pitch(FVector::RightVector, bDigging ? FMath::DegreesToRadians(16.0) : 0.0);
        const double Release = bExposed ? RearOpenRadians : 0.0;
        Head->SetRelativeRotation(Pitch * PresentationAlignment);
        LeftPlate->SetRelativeRotation(FQuat(RearHingeAxis, Release) * PresentationAlignment);
        RightPlate->SetRelativeRotation(FQuat(RearHingeAxis, -Release) * PresentationAlignment);
    }
    else
    {
        Head->SetRelativeLocation(FVector(173.0, 0.0, bDigging ? -75.0 : -30.0));
        Head->SetRelativeRotation(FRotator(bDigging ? -108.0 : -94.0, 0.0, 0.0));
        LeftPlate->SetRelativeLocation(FVector(-127.0, -91.0, bExposed ? 113.0 : 92.0));
        RightPlate->SetRelativeLocation(FVector(-127.0, 91.0, bExposed ? 113.0 : 92.0));
        LeftPlate->SetRelativeRotation(FRotator(-18.0, 0.0, bExposed ? -64.0 : -8.0));
        RightPlate->SetRelativeRotation(FRotator(-18.0, 0.0, bExposed ? 64.0 : 8.0));
    }

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
