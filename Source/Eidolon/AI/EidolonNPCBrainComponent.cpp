#include "EidolonNPCBrainComponent.h"
#include "EidolonAIPolicy.h"
FName UEidolonNPCBrainComponent::Evaluate(bool bRecruited, FName Order, bool bHostile, bool bSeen, bool bHeard, bool bAllyThreatened, bool bAllyCritical, bool bReinforcement, bool bInRange, bool bCanAct, bool bWork, bool bSocial, int32 HealthPercent)
{
 using namespace EidolonAI;
 Input X;
 const FName Roles[]={TEXT("civilian"),TEXT("guard"),TEXT("duelist"),TEXT("protector"),TEXT("mage"),TEXT("assassin"),TEXT("lancer"),TEXT("priest")};
 for(int32 I=0;I<UE_ARRAY_COUNT(Roles);++I) if(Role==Roles[I]) X.role=static_cast<EidolonAI::Role>(I);
 const FName Orders[]={TEXT("free"),TEXT("follow"),TEXT("hold"),TEXT("focus"),TEXT("retreat")};
 for(int32 I=0;I<UE_ARRAY_COUNT(Orders);++I) if(Order==Orders[I]) X.order=static_cast<EidolonAI::Order>(I);
 X.recruited=bRecruited; X.hostile=bHostile; X.seen=bSeen; X.heard=bHeard; X.allyThreatened=bAllyThreatened; X.allyCritical=bAllyCritical; X.reinforcement=bReinforcement; X.inRange=bInRange; X.canAct=bCanAct; X.work=bWork; X.social=bSocial; X.health=FMath::Clamp(HealthPercent,0,100);
 const FName Names[]={TEXT("idle"),TEXT("work"),TEXT("social"),TEXT("patrol"),TEXT("search"),TEXT("retreat"),TEXT("defend"),TEXT("reinforce"),TEXT("follow"),TEXT("hold"),TEXT("attack"),TEXT("flank"),TEXT("cast"),TEXT("heal")};
 const FName Next=Names[static_cast<int32>(Decide(X))];
 if(Next!=PreviousIntent){PreviousIntent=Next; OnIntentChanged.Broadcast(Next);}
 return Next;
}
