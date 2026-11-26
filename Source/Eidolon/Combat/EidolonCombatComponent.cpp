#include "Combat/EidolonCombatComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Combat/EidolonCombatProfiles.h"

using namespace EidolonCombat;
UEidolonCombatComponent::UEidolonCombatComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
}
bool UEidolonCombatComponent::Attack(bool bHeavy)
{
    const auto P=PlayerAttack(bHeavy);
    return StartAttack(P.Attack,P.Reach,P.Radius);
}
bool UEidolonCombatComponent::StartAttack(const FAttack& A,float Reach,float Radius)
{
    if(bPending || !GetWorld() || !GetOwner() || !FMath::IsFinite(Reach) || !FMath::IsFinite(Radius) || Reach<=0 || Radius<=0 || Radius>Reach) return false;
    const float Now=GetWorld()->GetTimeSeconds();
    if(!State.BeginAttack(A,Now)) return false;
    ArmAttack(A,Reach,Radius,Now); return true;
}
void UEidolonCombatComponent::ArmAttack(const FAttack& A,float Reach,float Radius,float Now)
{
    Pending=A; AttackReach=Reach; AttackRadius=Radius;
    ActiveStart=Now+A.Windup; ActiveEnd=ActiveStart+A.Active;
    HitActors.Reset(); bPending=true;
    OnTelegraph.Broadcast(A.Windup,Reach);
}
bool UEidolonCombatComponent::CanAct() const
{
    return GetWorld() && State.CanAct(GetWorld()->GetTimeSeconds());
}
bool UEidolonCombatComponent::CounterAttack()
{
    if(!GetWorld()) return false;
    auto P=PlayerAttack(false); P.Attack.Damage=32; P.Attack.Windup=.15f;
    const float Now=GetWorld()->GetTimeSeconds();
    if(!State.Counter(P.Attack,Now)) return false;
    ArmAttack(P.Attack,P.Reach,P.Radius,Now); return true;
}
bool UEidolonCombatComponent::InitializeStats(float H,float S,float M,float P)
{
    if(HasBegunPlay() || !FMath::IsFinite(H) || !FMath::IsFinite(S) || !FMath::IsFinite(M) || !FMath::IsFinite(P) || H<=0 || S<0 || M<0 || P<=0) return false;
    State.Health=State.MaxHealth=H; State.Stamina=State.MaxStamina=S;
    State.Mana=State.MaxMana=M; State.Poise=State.MaxPoise=P; return true;
}
bool UEidolonCombatComponent::IsInRange(const AActor* Target,float Reach) const
{
    if(!IsValid(Target) || !GetOwner() || !FMath::IsFinite(Reach) || Reach<=0) return false;
    return FVector::DistSquared(GetOwner()->GetActorLocation(),Target->GetActorLocation())<=FMath::Square(Reach);
}
bool UEidolonCombatComponent::Dodge()
{
    return GetWorld() && State.Dodge(GetWorld()->GetTimeSeconds());
}
bool UEidolonCombatComponent::BeginGuard()
{
    return GetWorld() && State.BeginBlock(GetWorld()->GetTimeSeconds());
}
void UEidolonCombatComponent::EndGuard(){ State.EndBlock(); }
void UEidolonCombatComponent::TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(Delta,TickType,ThisTick);
    if(!GetWorld() || !State.IsAlive()) { bPending=false; return; }
    const float Now=GetWorld()->GetTimeSeconds();
    State.Recover(Delta,Now);
    if(!bPending) return;
    if(Now<State.StaggerUntil) { bPending=false; return; }
    if(Now>=ActiveStart && Now<ActiveEnd) SampleHits();
    if(Now>=ActiveEnd) bPending=false;
}
void UEidolonCombatComponent::SampleHits()
{
    AActor* Owner=GetOwner();
    if(!Owner) return;
    const FVector Start=Owner->GetActorLocation(), End=Start+Owner->GetActorForwardVector()*FMath::Max(0.f,AttackReach-AttackRadius);
    FCollisionQueryParams Params; Params.AddIgnoredActor(Owner);
    FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_Pawn); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    TArray<FHitResult> Hits;
    GetWorld()->SweepMultiByObjectType(Hits,Start,End,FQuat::Identity,Objects,FCollisionShape::MakeSphere(AttackRadius),Params);
    for(const auto& Hit:Hits)
    {
        AActor* Target=Hit.GetActor();
        if(!IsInRange(Target,AttackReach) || HitActors.Contains(TWeakObjectPtr<AActor>(Target))) continue;
        auto* Combat=Target->FindComponentByClass<UEidolonCombatComponent>();
        if(!Combat || Combat->Team==Team || !Combat->State.IsAlive()) continue;
        FHitResult Wall;
        if(GetWorld()->LineTraceSingleByChannel(Wall,Start,Target->GetActorLocation(),ECC_Visibility,Params) && Wall.GetActor()!=Target) continue;
        if(!ClaimHit(HitActors,TWeakObjectPtr<AActor>(Target))) continue;
        Combat->ReceiveAttack(Pending,Owner);
        if(!bPending || !State.IsAlive()) break;
    }
}
void UEidolonCombatComponent::ReceiveAttack(const FAttack& A,AActor* Source)
{
    if(!GetWorld() || !IsValid(Source) || Source==GetOwner() || !GetOwner()) return;
    const auto* SourceCombat=Source->FindComponentByClass<UEidolonCombatComponent>();
    if(!SourceCombat || SourceCombat->Team==Team || !SourceCombat->State.IsAlive()) return;
    const float Now=GetWorld()->GetTimeSeconds();
    const FVector Direction=(Source->GetActorLocation()-GetOwner()->GetActorLocation()).GetSafeNormal2D();
    const bool Front=FVector::DotProduct(GetOwner()->GetActorForwardVector(),Direction)>=.5f;
    const bool Alive=State.IsAlive();
    SyncEncounter();
    if(!Encounter.CanTakeHealthDamage()) return;
    const auto Result=State.Receive(A,Now,Front);
    SyncEncounter();
    if(Now<State.StaggerUntil || !State.IsAlive()) bPending=false;
    if(Result==EOutcome::Parried)
        if(auto* Other=Source->FindComponentByClass<UEidolonCombatComponent>())
        { Other->bPending=false; Other->State.StaggerUntil=FMath::Max(Other->State.StaggerUntil,Now+.5f); }
    if(Result!=EOutcome::Ignored) OnImpact.Broadcast(Source,static_cast<int32>(Result));
    if(Alive && !State.IsAlive()) OnDeath.Broadcast();
}

void UEidolonCombatComponent::SyncEncounter()
{
    Encounter.bCustodian=bCustodian;
    Encounter.HealthFraction=State.Health/State.MaxHealth;
}
bool UEidolonCombatComponent::BreakCustodianAnchor(int32 Index)
{
    SyncEncounter();
    return Encounter.BreakAnchor(Index);
}
bool UEidolonCombatComponent::AttackEnemy(AActor* Target)
{
    SyncEncounter();
    if(!State.IsAlive()) return false;
    const auto P=EnemyAttack(Encounter.Phase());
    if(!IsInRange(Target,P.Reach)) return false;
    return StartAttack(P.Attack,P.Reach,P.Radius);
}
