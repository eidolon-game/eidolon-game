#pragma once
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <utility>
namespace EidolonRpg {
using Count = std::int64_t;
inline Count Threshold(int level) { if(level<1) level=1; if(level>100) level=100; return 50LL*(level-1)*(level+2); }
inline int Level(Count xp) { int l=1; while(l<100 && xp>=Threshold(l+1)) ++l; return l; }
struct RankFacts { int floor=0, giants=0; bool abyss=false,titan=false,council=false,memory=false,empyrea=false; };
inline const char* Rank(const RankFacts& f) {
 if(f.empyrea) return "Omega"; if(f.memory) return "EX"; if(f.council) return "SSS"; if(f.titan) return "SS"; if(f.giants>=100) return "S"; if(f.abyss) return "A"; if(f.floor>=80) return "B"; if(f.floor>=60) return "C"; if(f.floor>=40) return "D"; if(f.floor>=20) return "E"; return "F";
}
struct Item { Count price=0, stack=1; int level=1; bool unique=false; };
using Bag=std::map<std::string,Count>;
using Catalog=std::map<std::string,Item>;
struct Wallet { Count gold=0; Bag bag; };
inline Count Quantity(const Bag& b,const std::string& id) { auto i=b.find(id); return i==b.end()?0:i->second; }
inline bool Multiply(Count a,Count b,Count& out) { if(a<0||b<=0||a>std::numeric_limits<Count>::max()/b) return false; out=a*b; return true; }
inline bool Add(Wallet& w,const Catalog& c,const std::string& id,Count n) { auto i=c.find(id); if(i==c.end()||n<=0) return false; Count old=Quantity(w.bag,id),cap=i->second.unique?1:i->second.stack; if(old<0||old>cap||n>cap-old) return false; w.bag[id]=old+n; return true; }
inline bool Buy(Wallet& w,const Catalog& c,const std::string& id,Count n,int level) { auto i=c.find(id); Count cost; if(i==c.end()||i->second.unique||i->second.price<=0||level<i->second.level||!Multiply(i->second.price,n,cost)||w.gold<cost) return false; if(!Add(w,c,id,n)) return false; w.gold-=cost; return true; }
inline bool Sell(Wallet& w,const Catalog& c,const std::string& id,Count n) { auto i=c.find(id); Count value; if(i==c.end()||i->second.unique||i->second.price<=0||n<=0||Quantity(w.bag,id)<n||w.gold<0) return false; Count p=i->second.price; if(!Multiply((p/5)*2+(p%5)*2/5,n,value)||value>std::numeric_limits<Count>::max()-w.gold) return false; w.bag[id]-=n; if(!w.bag[id]) w.bag.erase(id); w.gold+=value; return true; }
struct Recipe { std::string output; Count count=1,gold=0; int level=1; Bag inputs; };
inline bool Craft(Wallet& w,const Catalog& c,const Recipe& r,int level) { if(r.gold<0||w.gold<r.gold||level<r.level||r.inputs.empty()) return false; Wallet next=w; for(const auto& p:r.inputs) { if(!c.count(p.first)||p.second<=0||Quantity(next.bag,p.first)<p.second) return false; next.bag[p.first]-=p.second; if(!next.bag[p.first]) next.bag.erase(p.first); } if(!Add(next,c,r.output,r.count)) return false; next.gold-=r.gold; w=std::move(next); return true; }
inline int Reputation(int current,int delta) { auto v=Count(current)+delta; return v<-100?-100:v>100?100:int(v); }
}
