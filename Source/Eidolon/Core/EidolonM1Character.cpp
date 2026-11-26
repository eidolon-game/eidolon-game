#include "EidolonM1Character.h"
#include "EidolonM1Adapter.h"
#include "GameFramework/Controller.h"

AEidolonM1Character::AEidolonM1Character()
{
    RuntimeAdapter = CreateDefaultSubobject<UEidolonM1Adapter>(TEXT("M1RuntimeAdapter"));
}
float AEidolonM1Character::TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer)
{
    const bool bWasDefeated = IsDefeated();
    if (!FMath::IsFinite(Amount) || Amount <= 0.f) return 0.f;
    const float Applied = Super::TakeDamage(Amount, Event, Instigator, Causer);
    if (HasAuthority() && !bWasDefeated && IsDefeated())
    {
        DefeatEventId = FGuid::NewGuid();
        // Aggregate source until spawn registry supplies stable instance identity.
        RuntimeAdapter->ObserveDeath(DefeatEventId, TEXT("m1_enemy_unknown"), TEXT("m1_player"), true);
    }
    return Applied;
}
