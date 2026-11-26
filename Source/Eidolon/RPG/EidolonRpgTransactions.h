#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
namespace EidolonRpgV2 {
using N=std::int64_t;
constexpr N Cap=1000000000, XpCap=504900;
using Bag=std::map<std::string,N>;
struct Item { N limit; int level; std::string slot; bool unique=false; };
using Catalog=std::map<std::string,Item>;
struct State { N gold=0,xp=0; Bag items; std::map<std::string,std::string> equipped; std::set<std::string> acquired; std::map<std::string,std::string> receipts; };
enum class Result { Applied, Duplicate, Conflict, Rejected };
struct Command { std::string id,source,target,quest,type; int chapter=0; N gold=0,xp=0; Bag items; std::map<std::string,std::string> equip; };
inline bool Id(const std::string& s) { if(s.empty()||s.size()>128)return false; for(char c:s)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return false; return true; }
inline int Level(N xp) { int l=1; while(l<100&&xp>=50LL*l*(l+3))++l; return l; }
inline N Qty(const Bag& b,const std::string& k){auto i=b.find(k);return i==b.end()?0:i->second;}
inline bool Valid(const State& s,const Catalog& c){
 if(s.gold<0||s.gold>Cap||s.xp<0||s.xp>XpCap||s.receipts.size()>100000)return false;
 for(auto& p:s.items){auto i=c.find(p.first);if(i==c.end()||p.second<=0||p.second>i->second.limit||(i->second.unique&&(p.second!=1||!s.acquired.count(p.first))))return false;}
 for(auto& p:s.acquired){auto i=c.find(p);if(i==c.end()||!i->second.unique)return false;}
 for(auto& p:s.equipped){auto i=c.find(p.second);if(i==c.end()||i->second.slot!=p.first||!(p.first=="weapon"||p.first=="body")||Qty(s.items,p.second)<1||Level(s.xp)<i->second.level)return false;}
 return true;
}
inline std::string Fingerprint(const Command& c){
 std::string s;auto add=[&](const std::string& v){s+=std::to_string(v.size())+":"+v;};
 for(auto& v:{c.source,c.target,c.quest,c.type})add(v);
 for(N v:{N(c.chapter),c.gold,c.xp,N(c.items.size()),N(c.equip.size())})add(std::to_string(v));
 for(auto& p:c.items){add(p.first);add(std::to_string(p.second));}
 for(auto& p:c.equip){add(p.first);add(p.second);}return s;
}
// Trusted callers must validate event provenance, current chapter and entitlement.
// Save commits the returned candidate together with quest progress before acknowledgement.
inline Result Apply(State& s,const Catalog& catalog,const Command& c,int activeChapter){
 if(!Id(c.id)||!Id(c.source)||!Id(c.target)||!Id(c.quest)||c.chapter<1||c.chapter>26||!Valid(s,catalog))return Result::Rejected;
 auto fp=Fingerprint(c);auto receipt=s.receipts.find(c.id);
 if(receipt!=s.receipts.end())return receipt->second==fp?Result::Duplicate:Result::Conflict;
 if(c.chapter!=activeChapter||s.receipts.size()>=100000||c.gold<0||c.gold>Cap||c.xp<0||c.xp>XpCap)return Result::Rejected;
 State next=s;
 if(c.type=="grant"){
  if(c.gold>Cap-next.gold)return Result::Rejected;
  next.gold+=c.gold;next.xp=c.xp>XpCap-next.xp?XpCap:next.xp+c.xp;
  for(auto& p:c.items){auto i=catalog.find(p.first);if(i==catalog.end()||p.second<=0||p.second>i->second.limit-Qty(next.items,p.first)||(i->second.unique&&(p.second!=1||next.acquired.count(p.first))))return Result::Rejected;next.items[p.first]+=p.second;if(i->second.unique)next.acquired.insert(p.first);}
 }else if(c.type=="spend"){
  if(c.xp||!c.equip.empty()||c.gold>next.gold)return Result::Rejected;next.gold-=c.gold;
  for(auto& p:c.items){if(p.second<=0||Qty(next.items,p.first)<p.second)return Result::Rejected;auto& n=next.items[p.first];n-=p.second;if(!n)next.items.erase(p.first);}
 }else if(c.type=="equip"){if(c.gold||c.xp||!c.items.empty()||c.equip.empty())return Result::Rejected;}
 else return Result::Rejected;
 for(auto& p:c.equip){if(p.second.empty())next.equipped.erase(p.first);else next.equipped[p.first]=p.second;}
 if(!Valid(next,catalog))return Result::Rejected;
 next.receipts.emplace(c.id,std::move(fp));s=std::move(next);return Result::Applied;
}
}
