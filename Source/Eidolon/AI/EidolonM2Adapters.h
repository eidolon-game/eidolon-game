#pragma once
#include "EidolonM2Policy.h"
#include <string_view>
namespace EidolonAI::M2 {
// Boundary adapter: quest IDs are not Save entity IDs.
inline std::string_view QuestId(Actor a) {
 switch(a){case Actor::Darius:return "darius";case Actor::Elara:return "elara";case Actor::Lyra:return "lyra";case Actor::Noah:return "noah";case Actor::Guild:return "aldren";case Actor::Monster:return "tutorial_monster";}return {};
}
inline std::string_view SaveId(Actor a) {
 switch(a){case Actor::Darius:return "npc_darius";case Actor::Elara:return "npc_elara";case Actor::Lyra:return "npc_lyra";case Actor::Noah:return "npc_noah";default:return {};}
}
struct CombatView {bool sourceValid=false,targetValid=false; int sourceTeam=0,targetTeam=0; double sourceHealth=0,targetHealth=0,distanceCm=0; bool visible=false,inFov=false,canAct=false;};
inline Observation Observe(const CombatView& v,bool heard,double soundAge) {
 return {v.sourceValid&&v.targetValid,v.targetHealth>0,v.sourceTeam!=v.targetTeam,v.visible,v.inFov,heard,v.distanceCm,soundAge};
}
// A caller must revalidate actor identity, range, line of sight and combat action
// immediately before requesting StartAttack; never apply damage from AI.
inline bool RequestAttack(const Input& x,const CombatView& v) {
 return x.actor==Actor::Monster && x.healthPercent>0 && v.sourceHealth>0 && v.canAct &&
 v.sourceValid&&v.targetValid&&v.targetHealth>0&&v.sourceTeam!=v.targetTeam&&v.visible&&v.inFov&&
 std::isfinite(v.distanceCm)&&v.distanceCm>=0&&v.distanceCm<=x.profile.reachCm&&Decide(x)==Intent::Attack;
}
// Event proposal only: Core supplies GUID, domain and receipt; Save commits.
// No quest completion or reward side effect is permitted here.
struct IntentEnvelope {std::string_view source,target,semantic; int chapter=0,expectedRevision=-1;};
inline IntentEnvelope Propose(const Input& x,std::string_view target) {
 return {QuestId(x.actor),target.empty()?QuestId(x.actor):target,Semantic(Decide(x)),x.snapshot.chapter,x.snapshot.revision};
}
}
