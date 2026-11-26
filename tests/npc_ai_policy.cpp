#include "../Source/Eidolon/AI/EidolonAIPolicy.h"
#include <cassert>
using namespace EidolonAI;
int main() {
 Input x; x.work=true; assert(Decide(x)==Intent::Work);
 x.hostile=x.seen=true; assert(Decide(x)==Intent::Retreat);
 x.role=Role::Protector; x.allyThreatened=true; assert(Decide(x)==Intent::Defend);
 x.role=Role::Priest; x.allyCritical=true; assert(Decide(x)==Intent::Heal);
 x.role=Role::Assassin; assert(Decide(x)==Intent::Flank);
 x.role=Role::Mage; x.inRange=true; assert(Decide(x)==Intent::Cast);
 x.seen=false; x.heard=true; assert(Decide(x)==Intent::Search);
 x.recruited=true; x.order=Order::Hold; assert(Decide(x)==Intent::Hold);
 x.health=0; assert(Decide(x)==Intent::Idle);
 State s; assert(!s.Recruit(false)); assert(s.Recruit(true));
 assert(s.Relationship("choice.1",100,-100)); assert(!s.Relationship("choice.1",100,100));
 assert(s.affinity==100 && s.loyalty==-100); assert(!s.Leave(false)); assert(s.Leave(true));
 assert(s.Rewrite("noah",true,false,1)); assert(s.Rewrite("noah",false,true,2));
 assert(!s.Rewrite("noah",true,false,1)); assert(s.memories.at("noah").retained);
 State family; assert(family.Rewrite("noah",false,false,2)); assert(!family.memories.at("noah").retained);
}
