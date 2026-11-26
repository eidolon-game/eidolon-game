#include "EidolonM1Adapter.h"
#include "EidolonM1EventSubsystem.h"
#include "../EidolonCharacter.h"
#include "Engine/World.h"

FEidolonM1Snapshot UEidolonM1Adapter::Capture() const
{
    FEidolonM1Snapshot Result;
    if (const auto* Player = Cast<AEidolonCharacter>(GetOwner()))
    {
        const float H = Player->GetHealthFraction(), S = Player->GetStaminaFraction();
        if (!FMath::IsFinite(H) || !FMath::IsFinite(S)) return Result;
        Result.HealthFraction = FMath::Clamp(H, 0.f, 1.f);
        Result.StaminaFraction = FMath::Clamp(S, 0.f, 1.f);
        Result.bDefeated = Player->IsDefeated();
        Result.bAvailable = true;
    }
    return Result;
}
EEidolonReceipt UEidolonM1Adapter::ObserveDeath(const FGuid& EventId, const FString& SourceId,
    const FString& TargetId, bool bPlayerDefeated)
{
    if (!IsInGameThread() || !GetOwner() || !GetOwner()->HasAuthority() || !GetWorld())
        return EEidolonReceipt::Invalid;
    auto* Bus = GetWorld()->GetSubsystem<UEidolonM1EventSubsystem>();
    if (!Bus) return EEidolonReceipt::Invalid;
    FEidolonRuntimeEvent Event;
    Event.EventId = EventId;
    Event.SourceId = SourceId;
    Event.TargetId = TargetId;
    Event.Kind = bPlayerDefeated ? EEidolonEventKind::PlayerDefeated : EEidolonEventKind::EnemyDefeated;
    Event.SemanticId = bPlayerDefeated ? TEXT("player_defeated") : TEXT("enemy_defeated");
    return Bus->Publish(Event);
}
