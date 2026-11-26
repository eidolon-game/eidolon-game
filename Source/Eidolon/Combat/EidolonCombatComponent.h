#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/EidolonCombatRules.h"
#include "Combat/EidolonEnemyEncounter.h"
#include "EidolonCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEidolonCombatImpact,AActor*,Source,int32,Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEidolonCombatDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEidolonCombatTelegraph,float,Windup,float,Reach);

UCLASS(ClassGroup=(Eidolon), meta=(BlueprintSpawnableComponent))
class EIDOLON_API UEidolonCombatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UEidolonCombatComponent();
    virtual void TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* ThisTick) override;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Combat") int32 Team=0;
    UPROPERTY(BlueprintAssignable,Category="Combat") FEidolonCombatImpact OnImpact;
    UPROPERTY(BlueprintAssignable,Category="Combat") FEidolonCombatDeath OnDeath;
    UPROPERTY(BlueprintAssignable,Category="Combat") FEidolonCombatTelegraph OnTelegraph;
    UFUNCTION(BlueprintPure,Category="Combat") bool CanAct() const;
    UFUNCTION(BlueprintCallable,Category="Combat") bool CounterAttack();
    bool IsInRange(const AActor* Target,float Reach) const;
    UFUNCTION(BlueprintCallable,Category="Combat") bool AttackEnemy(AActor* Target);
    UFUNCTION(BlueprintCallable,Category="Combat") bool BreakCustodianAnchor(int32 Index);
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Combat") bool bCustodian=false;
    bool InitializeStats(float Health,float Stamina,float Mana,float Poise);
    UFUNCTION(BlueprintCallable,Category="Combat") bool Attack(bool bHeavy);
    UFUNCTION(BlueprintCallable,Category="Combat") bool Dodge();
    UFUNCTION(BlueprintCallable,Category="Combat") bool BeginGuard();
    UFUNCTION(BlueprintCallable,Category="Combat") void EndGuard();
    UFUNCTION(BlueprintPure,Category="Combat") float Health() const { return State.Health; }
    UFUNCTION(BlueprintPure,Category="Combat") float Stamina() const { return State.Stamina; }
    UFUNCTION(BlueprintPure,Category="Combat") float Mana() const { return State.Mana; }
    bool StartAttack(const EidolonCombat::FAttack& Attack,float Reach,float Radius);
    void ReceiveAttack(const EidolonCombat::FAttack& Attack,AActor* Source);
private:
    EidolonCombat::FState State;
    EidolonCombat::FEnemyEncounter Encounter;
    EidolonCombat::FAttack Pending;
    float ActiveStart=0,ActiveEnd=0,AttackReach=0,AttackRadius=0;
    bool bPending=false;
    TSet<TWeakObjectPtr<AActor>> HitActors;
    void SampleHits();
    void SyncEncounter();
    void ArmAttack(const EidolonCombat::FAttack& A,float Reach,float Radius,float Now);
};
