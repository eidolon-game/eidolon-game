#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EidolonLegacyCombatAdapter.generated.h"
class UEidolonCombatComponent;
UCLASS(ClassGroup=(Eidolon),meta=(BlueprintSpawnableComponent))
class EIDOLON_API UEidolonLegacyCombatAdapter : public UActorComponent
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable) bool ExecuteIntent(FName Intent,AActor* Target);
 UFUNCTION(BlueprintCallable) bool DodgePawn();
 UFUNCTION(BlueprintCallable) bool AttackPawn(bool Heavy);
 UFUNCTION(BlueprintCallable) void ReleaseGuard();
private:
 UEidolonCombatComponent* Combat() const;
};
