#pragma once
#include <cmath>
#include <string_view>
namespace EidolonAI::M2 {
enum class Actor { Darius, Elara, Lyra, Noah, Guild, Monster };
enum class Presence { Absent, Present, Oblivion };
enum class Reality { Prime, Oblivion, Alternate };
enum class Intent { Dormant, Dead, Rest, Forge, HealWork, Train, Play, Social, GuildDuty, Patrol, Investigate, Pursue, Attack, Wait, Flee };
struct Snapshot {
 bool committed=false; int chapter=0, revision=-1; Presence noah=Presence::Absent; Reality reality=Reality::Prime;
 bool dormitoryComplete=false;
};
inline bool Visible(Actor a,const Snapshot& s) {
 if(!s.committed || s.chapter<1 || s.chapter>26 || s.revision<0 || s.reality==Reality::Alternate) return false;
 if(a==Actor::Noah) return (s.reality==Reality::Prime && s.noah==Presence::Present && (s.chapter>2 || (s.chapter==2 && s.dormitoryComplete))) || (s.reality==Reality::Oblivion && s.noah==Presence::Oblivion && s.chapter>=22);
 return s.reality==Reality::Prime;
}
struct Observation {
 bool valid=false, alive=false, hostile=false, lineOfSight=false, inFov=false, heard=false;
 double distanceCm=0, soundAgeSeconds=0;
};
struct Profile { double sightCm=1200, hearingCm=800, hearingSeconds=3, reachCm=150; int fleeHealth=20; };
struct Input { Actor actor=Actor::Darius; Snapshot snapshot; Observation target; Profile profile; int minute=0, healthPercent=100; bool canAct=true; };
inline bool ValidProfile(const Profile& p) {
 return std::isfinite(p.sightCm)&&std::isfinite(p.hearingCm)&&std::isfinite(p.hearingSeconds)&&std::isfinite(p.reachCm)&&p.sightCm>0&&p.hearingCm>=0&&p.hearingSeconds>=0&&p.reachCm>0&&p.reachCm<=p.sightCm&&p.fleeHealth>=0&&p.fleeHealth<=100;
}
inline Intent Routine(Actor a,int minute) {
 if(minute<0||minute>=1440) return Intent::Rest;
 if(a==Actor::Monster) return Intent::Patrol;
 if(minute<480||minute>=1320) return Intent::Rest;
 if(minute>=1080) return Intent::Social;
 switch(a) {
 case Actor::Darius:return Intent::Forge; case Actor::Elara:return Intent::HealWork;
 case Actor::Lyra:return Intent::Train; case Actor::Noah:return Intent::Play;
 case Actor::Guild:return Intent::GuildDuty; default:return Intent::Patrol;
 }
}
inline Intent Decide(const Input& x) {
 if(x.healthPercent<=0) return Intent::Dead;
 if(!Visible(x.actor,x.snapshot)||!ValidProfile(x.profile)||x.healthPercent>100) return Intent::Dormant;
 const auto& t=x.target; const auto& p=x.profile;
 const bool valid=t.valid&&t.alive&&t.hostile&&std::isfinite(t.distanceCm)&&t.distanceCm>=0;
 const bool seen=valid&&t.lineOfSight&&t.inFov&&t.distanceCm<=p.sightCm;
 const bool heard=valid&&t.heard&&std::isfinite(t.soundAgeSeconds)&&t.soundAgeSeconds>=0&&t.soundAgeSeconds<=p.hearingSeconds&&t.distanceCm<=p.hearingCm;
 if(seen||heard) {
  if(x.actor!=Actor::Monster||x.healthPercent<=p.fleeHealth) return Intent::Flee;
  if(!seen) return Intent::Investigate;
  if(t.distanceCm>p.reachCm) return Intent::Pursue;
  return x.canAct?Intent::Attack:Intent::Wait;
 }
 return Routine(x.actor,x.minute);
}
inline std::string_view Semantic(Intent i) {
 switch(i) {
 case Intent::Dormant:return "dormant";case Intent::Dead:return "dead";case Intent::Rest:return "rest";case Intent::Forge:return "forge";case Intent::HealWork:return "heal_work";case Intent::Train:return "train";case Intent::Play:return "play";case Intent::Social:return "social";case Intent::GuildDuty:return "guild_duty";case Intent::Patrol:return "patrol";case Intent::Investigate:return "investigate";case Intent::Pursue:return "pursue";case Intent::Attack:return "attack";case Intent::Wait:return "wait";case Intent::Flee:return "flee";
 } return "dormant";
}
inline bool CanInteract(const Input& x,double distanceCm,bool lineOfSight) {
 return x.actor!=Actor::Monster && x.healthPercent>0 && x.healthPercent<=100 && Visible(x.actor,x.snapshot)&&std::isfinite(distanceCm)&&distanceCm>=0&&distanceCm<=200&&lineOfSight&&Decide(x)!=Intent::Flee&&Decide(x)!=Intent::Dormant;
}
}
