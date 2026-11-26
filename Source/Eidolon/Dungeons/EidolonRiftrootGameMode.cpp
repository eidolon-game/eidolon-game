#include "EidolonRiftrootGameMode.h"

#include "EidolonAbyssActorBindings.h"
#include "EidolonRiftrootTangle.h"
#include "Engine/TargetPoint.h"

FEidolonAbyssEncounter AEidolonRiftrootGameMode::GetEncounter() const
{
    using namespace EidolonAbyss;
    return {
        RiftrootFloor, TEXT("abyss_020_boss_spawn"), TEXT("abyss_020_riftroot_gate"),
        AEidolonRiftrootTangle::StaticClass(), 165.0, false,
        TEXT("Roots in the Fissure | Abyss, floor 20"), TEXT("Radici nella faglia | Abisso, livello 20"),
        TEXT("Leave the rock wedge's path. Keep the opposite flank clear of the roots."),
        TEXT("Esci dalla traiettoria del cuneo di roccia. Cerca il fianco opposto alle radici tese."),
        EventKind::RiftrootDefeated, Target::Riftroot
    };
}

bool AEidolonRiftrootGameMode::ConfigureEnemy(AEidolonArenaEnemy& Enemy)
{
    using EidolonAbyss::FindTaggedActor;
    auto* Boss = Cast<AEidolonRiftrootTangle>(&Enemy);
    auto* World = GetWorld();
    if (!Boss || !World) return false;

    const auto* Spawn = FindTaggedActor<ATargetPoint>(*World, GetEncounter().SpawnTag);
    const auto* Advance = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_020_boss_advance"));
    return Spawn && Advance && Boss->ConfigureFaultAdvance(Spawn->GetActorLocation(), Advance->GetActorLocation());
}
