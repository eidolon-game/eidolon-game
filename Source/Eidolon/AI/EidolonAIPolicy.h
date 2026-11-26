#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <map>
#include <set>
namespace EidolonAI {
enum class Intent { Idle, Work, Social, Patrol, Search, Retreat, Defend, Reinforce, Follow, Hold, Attack, Flank, Cast, Heal };
enum class Role { Civilian, Guard, Duelist, Protector, Mage, Assassin, Lancer, Priest };
enum class Order { Free, Follow, Hold, Focus, Retreat };
struct Input {
 Role role=Role::Civilian; Order order=Order::Free;
 bool recruited=false, hostile=false, seen=false, heard=false, allyThreatened=false, allyCritical=false, reinforcement=false, inRange=false, canAct=true, work=false, social=false;
 int health=100; int loyalty=0;
};
inline Intent Decide(const Input& x) {
 if(x.health<=0) return Intent::Idle;
 if(x.health<=20 || (x.recruited && x.order==Order::Retreat)) return Intent::Retreat;
 if(x.hostile && x.seen) {
  if(x.role==Role::Civilian) return Intent::Retreat;
  if(x.role==Role::Protector && x.allyThreatened) return Intent::Defend;
  if(x.role==Role::Priest && x.allyCritical && x.canAct) return Intent::Heal;
  if(x.recruited && x.order==Order::Hold) return x.inRange && x.canAct ? Intent::Attack : Intent::Hold;
  if(!x.canAct) return Intent::Defend;
  if(x.role==Role::Assassin && !x.inRange) return Intent::Flank;
  if(x.role==Role::Mage && x.inRange) return Intent::Cast;
  return Intent::Attack;
 }
 if(x.role==Role::Priest && x.allyCritical && x.canAct) return Intent::Heal;
 if(x.recruited && x.order==Order::Hold) return Intent::Hold;
 if(x.hostile && x.heard && x.role!=Role::Civilian) return Intent::Search;
 if(x.reinforcement && x.role==Role::Guard) return Intent::Reinforce;
 if(x.recruited) return Intent::Follow;
 if(x.work) return Intent::Work;
 if(x.social) return Intent::Social;
 return x.role==Role::Guard ? Intent::Patrol : Intent::Idle;
}
struct Memory { bool visible=true; bool retained=false; std::uint64_t revision=0; };
struct State {
 int affinity=0, loyalty=0; bool recruited=false; std::uint64_t revision=0;
 std::map<std::string,Memory> memories; std::set<std::string> appliedEvents;
 bool Relationship(const std::string& event,int affinityDelta,int loyaltyDelta) {
  if(event.empty() || !appliedEvents.insert(event).second) return false;
  affinity=std::clamp(affinity+std::clamp(affinityDelta,-100,100),-100,100);
  loyalty=std::clamp(loyalty+std::clamp(loyaltyDelta,-100,100),-100,100);
  return true;
 }
 bool Rewrite(const std::string& id,bool present,bool retain,std::uint64_t rev) {
  if(id.empty() || rev==0 || rev<revision) return false;
  auto it=memories.find(id);
  if(it!=memories.end() && it->second.revision>=rev) return false;
  revision=rev; memories[id]={present,retain,rev}; return true;
 }
 bool Recruit(bool storyAllowed) { if(!storyAllowed || loyalty<0 || recruited) return false; recruited=true; return true; }
 bool Leave(bool storyAllowed) { if(!storyAllowed || !recruited || loyalty>-60) return false; recruited=false; return true; }
};
}
