#include "Combat/EidolonLegacyCombatAdapter.h"
#include "Combat/EidolonCombatComponent.h"
#include "Combat/EidolonCombatProfiles.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
UEidolonCombatComponent* UEidolonLegacyCombatAdapter::Combat() const
{
 return GetOwner()?GetOwner()->FindComponentByClass<UEidolonCombatComponent>():nullptr;
}
bool UEidolonLegacyCombatAdapter::AttackPawn(bool Heavy)
{
 auto* C=Combat(); return C && C->Attack(Heavy);
}
bool UEidolonLegacyCombatAdapter::DodgePawn()
{
 auto* Pawn=Cast<ACharacter>(GetOwner()); auto* C=Combat();
 if(!Pawn || !C || !Pawn->GetCharacterMovement()->IsMovingOnGround() || !C->Dodge()) return false;
 FVector Dir=Pawn->GetLastMovementInputVector().GetSafeNormal2D();
 if(Dir.IsNearlyZero()) Dir=Pawn->GetActorForwardVector();
 Pawn->LaunchCharacter(Dir*650.f,true,false); return true;
}
void UEidolonLegacyCombatAdapter::ReleaseGuard(){ if(auto* C=Combat()) C->EndGuard(); }
bool UEidolonLegacyCombatAdapter::ExecuteIntent(FName Intent,AActor* Target)
{
 auto* C=Combat(); if(!C) return false;
 if(Intent==TEXT("defend")) return C->BeginGuard();
 if(Intent!=TEXT("counter")) C->EndGuard();
 if(Intent!=TEXT("attack") && Intent!=TEXT("counter")) return false;
 const auto P=EidolonCombat::PlayerAttack(false);
 if(Intent==TEXT("counter") && !C->IsInRange(Target,P.Reach)) return false;
 return Intent==TEXT("counter")?C->CounterAttack():C->AttackEnemy(Target);
}
