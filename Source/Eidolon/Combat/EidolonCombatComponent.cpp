#include "Combat/EidolonCombatComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

using namespace EidolonCombat;
UEidolonCombatComponent::UEidolonCombatComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
}
bool UEidolonCombatComponent::Attack(bool bHeavy)
{
    FAttack A;
    if(bHeavy){ A.Damage=40; A.Stamina=28; A.Poise=45; A.Windup=.55f; A.Recovery=.65f; }
    return StartAttack(A,220.f,45.f);
}
bool UEidolonCombatComponent::StartAttack(const FAttack& A,float Reach,float Radius)
{
    if(!GetWorld() || !GetOwner() || !FMath::IsFinite(Reach) || !FMath::IsFinite(Radius) || Reach<=0 || Radius<=0) return false;
    const float Now=GetWorld()->GetTimeSeconds();
    if(!State.BeginAttack(A,Now)) return false;
    Pending=A; AttackReach=Reach; AttackRadius=Radius;
    ActiveStart=Now+A.Windup; ActiveEnd=ActiveStart+A.Active; LastSample=Now;
    HitActors.Reset(); bPending=true; return true;
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
    if(Now>=ActiveStart && LastSample<ActiveEnd) SampleHits();
    LastSample=Now;
    if(Now>=ActiveEnd) bPending=false;
}
void UEidolonCombatComponent::SampleHits()
{
    AActor* Owner=GetOwner();
    if(!Owner) return;
    const FVector Start=Owner->GetActorLocation(), End=Start+Owner->GetActorForwardVector()*AttackReach;
    FCollisionQueryParams Params; Params.AddIgnoredActor(Owner);
    FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_Pawn); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    TArray<FHitResult> Hits;
    GetWorld()->SweepMultiByObjectType(Hits,Start,End,FQuat::Identity,Objects,FCollisionShape::MakeSphere(AttackRadius),Params);
    for(const auto& Hit:Hits)
    {
        AActor* Target=Hit.GetActor();
        if(!IsValid(Target) || HitActors.Contains(TWeakObjectPtr<AActor>(Target))) continue;
        auto* Combat=Target->FindComponentByClass<UEidolonCombatComponent>();
        if(!Combat || Combat->Team==Team || !Combat->State.IsAlive()) continue;
        FHitResult Wall;
        if(GetWorld()->LineTraceSingleByChannel(Wall,Start,Target->GetActorLocation(),ECC_Visibility,Params) && Wall.GetActor()!=Target) continue;
        HitActors.Add(TWeakObjectPtr<AActor>(Target));
        Combat->ReceiveAttack(Pending,Owner);
        if(!bPending || !State.IsAlive()) break;
    }
}
void UEidolonCombatComponent::ReceiveAttack(const FAttack& A,AActor* Source)
{
    if(!GetWorld() || !IsValid(Source) || Source==GetOwner() || !GetOwner()) return;
    const float Now=GetWorld()->GetTimeSeconds();
    const FVector Direction=(Source->GetActorLocation()-GetOwner()->GetActorLocation()).GetSafeNormal2D();
    const bool Front=FVector::DotProduct(GetOwner()->GetActorForwardVector(),Direction)>=.5f;
    const bool Alive=State.IsAlive();
    const auto Result=State.Receive(A,Now,Front);
    if(Now<State.StaggerUntil || !State.IsAlive()) bPending=false;
    if(Result==EOutcome::Parried)
        if(auto* Other=Source->FindComponentByClass<UEidolonCombatComponent>())
        { Other->bPending=false; Other->State.StaggerUntil=FMath::Max(Other->State.StaggerUntil,Now+.5f); }
    if(Result!=EOutcome::Ignored) OnImpact.Broadcast(Source,static_cast<int32>(Result));
    if(Alive && !State.IsAlive()) OnDeath.Broadcast();
}
