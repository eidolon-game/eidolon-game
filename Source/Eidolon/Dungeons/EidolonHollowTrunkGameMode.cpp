#include "EidolonHollowTrunkGameMode.h"

#include "../EidolonCharacter.h"
#include "CollisionQueryParams.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonForestStalker.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"

namespace
{
    constexpr double PinHeightCm = 28.0;

    // Envelope of the connected carrying area before the bridge lowers.
    bool IsInCarryingArea(const FVector& Location)
    {
        return !Location.ContainsNaN() && Location.X >= 200.0 && Location.X <= 3400.0
            && Location.Y >= 0.0 && Location.Y <= 2200.0 && Location.Z >= -70.0;
    }

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
            ? Italian : English;
    }
}

AEidolonHollowTrunkGameMode::AEidolonHollowTrunkGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
}

FEidolonAbyssEncounter AEidolonHollowTrunkGameMode::GetEncounter() const
{
    using namespace EidolonAbyss;
    FEidolonAbyssEncounter Floor;
    Floor.Floor = HollowTrunkFloor;
    Floor.GateTag = TEXT("abyss_015_gate");
    Floor.TitleEnglish = TEXT("Hollow Trunk Plates | Abyss, floor 15");
    Floor.TitleItalian = TEXT("Piastre nel tronco | Abisso, livello 15");
    Floor.CompletionKind = EventKind::HollowTrunkPinSeated;
    Floor.CompletionTarget = Target::HollowTrunkSocket;
    Floor.CompletionControlTag = TEXT("abyss_015_pin_seat");
    return Floor;
}

bool AEidolonHollowTrunkGameMode::BindCompletionActors(UWorld& World)
{
    using EidolonAbyss::FindTaggedActor;
    Alcove = FindTaggedActor<ATargetPoint>(World, TEXT("abyss_015_pin_pickup"));
    Socket = FindTaggedActor<ATargetPoint>(World, GetEncounter().CompletionControlTag);
    Pin = FindTaggedActor<AStaticMeshActor>(World, TEXT("abyss_015_pin"));
    Walkway = FindTaggedActor<AStaticMeshActor>(World, TEXT("abyss_015_walkway"));
    if (!Alcove.IsValid() || !Socket.IsValid() || !Pin.IsValid() || !Walkway.IsValid()) return false;

    AlcovePose = Pin->GetActorTransform();
    Pin->SetActorEnableCollision(false);
    return true;
}

void AEidolonHollowTrunkGameMode::UpdateCompletionActors(bool bCompleted)
{
    if (auto* Bridge = Walkway.Get())
    {
        Bridge->SetActorEnableCollision(bCompleted);
        Bridge->SetActorHiddenInGame(!bCompleted);
    }
    if (bCompleted)
    {
        if (const auto* Seat = Socket.Get()) PlacePin(Seat->GetActorLocation(), Seat->GetActorQuat());
    }
}

bool AEidolonHollowTrunkGameMode::InteractWithControl(AEidolonCharacter& Player, const ATargetPoint& Control)
{
    auto* Prop = Pin.Get();
    if (!Prop || !Walkway.IsValid() || Player.IsDodging() || !Player.GetCharacterMovement()->IsMovingOnGround())
        return false;

    if (Carrier.Get() == &Player)
    {
        if (Prop->GetAttachParentActor() != &Player)
        {
            ReturnPinToAlcove();
            return false;
        }
        if (IsInCarryingArea(Player.GetActorLocation()) && CanInteractAt(Player, &Control)) return true;
        DropPin(Player);
        return false;
    }
    if (!Carrier.IsExplicitlyNull()) return false;

    if (CanInteractAt(Player, Prop))
    {
        if (Prop->AttachToActor(&Player, FAttachmentTransformRules::KeepWorldTransform))
        {
            Carrier = &Player;
            Prop->SetActorRelativeLocation(FVector(-65.0, 0.0, -33.0));
            Prop->SetActorRelativeRotation(FRotator::ZeroRotator);
            bRecovered = false;
            ClearNotice();
        }
    }
    else if (!Prop->GetActorLocation().Equals(AlcovePose.GetLocation(), 1.0) && CanInteractAt(Player, Alcove.Get()))
    {
        ReturnPinToAlcove();
    }
    return false;
}

void AEidolonHollowTrunkGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Carrier.IsExplicitlyNull()) return;

    auto* Player = Carrier.Get();
    if (!Player || Player->IsDefeated() || !Player->IsPlayerControlled() || !IsInCarryingArea(Player->GetActorLocation()))
        ReturnPinToAlcove();
    else if (Player->IsDodging())
        DropPin(*Player);
}

void AEidolonHollowTrunkGameMode::PlacePin(const FVector& Location, const FQuat& Rotation)
{
    Carrier.Reset();
    if (auto* Prop = Pin.Get())
    {
        Prop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Prop->SetActorTransform(FTransform(Rotation, Location, AlcovePose.GetScale3D()));
    }
}

void AEidolonHollowTrunkGameMode::ReturnPinToAlcove()
{
    PlacePin(AlcovePose.GetLocation(), AlcovePose.GetRotation());
    bRecovered = true;
    ClearNotice();
}

void AEidolonHollowTrunkGameMode::DropPin(AEidolonCharacter& Player)
{
    const FVector Origin = Player.GetActorLocation();
    const FCollisionObjectQueryParams Surface(ECC_WorldStatic);
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(EidolonHollowTrunkDrop), false, &Player);
    FHitResult Hit;
    if (IsInCarryingArea(Origin) && GetWorld()->LineTraceSingleByObjectType(Hit, Origin + FVector(0, 0, 45),
            Origin - FVector(0, 0, 155), Surface, Query) && Hit.ImpactNormal.Z >= 0.7
        && IsInCarryingArea(Hit.ImpactPoint))
    {
        PlacePin(Hit.ImpactPoint + FVector(0, 0, PinHeightCm), AlcovePose.GetRotation());
        ClearNotice();
    }
    else
    {
        ReturnPinToAlcove();
    }
}

FString AEidolonHollowTrunkGameMode::GetControlPrompt() const
{
    if (Carrier.IsValid())
        return Display(TEXT("Bring the pin to the bridge. Interact away from the socket to set it down."),
            TEXT("Porta il perno alla passerella. Interagisci lontano dall'alloggio per posarlo."));
    if (bRecovered)
        return Display(TEXT("The pin is back in the alcove."), TEXT("Il perno è tornato nella nicchia."));
    return Display(TEXT("Pick up the pin. Interact at the alcove to retrieve it if you lose it."),
        TEXT("Raccogli il perno. Se lo perdi, interagisci alla nicchia per recuperarlo."));
}

void AEidolonHollowTrunkGameMode::SpawnOptionalThreats()
{
    auto* World = GetWorld();
    if (!World) return;
    const auto* Anchor = EidolonAbyss::FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_015_stalker_spawn"));
    if (!Anchor) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    World->SpawnActor<AEidolonForestStalker>(Anchor->GetActorLocation() + FVector(0, 0, 95.0),
        Anchor->GetActorRotation(), Params);
}
