#pragma once
#include "Combat/EidolonCombatProfiles.h"
namespace EidolonCombat
{
// Encounter-local state. Only the validated objective owner may break anchors.
struct FEnemyEncounter
{
 bool bCustodian=false;
 uint8 BrokenMask=0;
 float HealthFraction=1.f;
 bool BreakAnchor(int32 Index)
 {
  if(!bCustodian || HealthFraction<=0 || Index<0 || Index>=3 || (BrokenMask&(1<<Index))) return false;
  BrokenMask|=1<<Index; return true;
 }
 int32 RemainingAnchors() const { return 3-((BrokenMask&1)!=0)-((BrokenMask&2)!=0)-((BrokenMask&4)!=0); }
 EEnemyPhase Phase() const
 {
  if(HealthFraction<=0) return EEnemyPhase::Defeated;
  return bCustodian?CustodianPhase(HealthFraction,RemainingAnchors()):EEnemyPhase::FirstMonster;
 }
 bool CanTakeHealthDamage() const { return Phase()!=EEnemyPhase::CustodianBound && Phase()!=EEnemyPhase::Defeated; }
};
}
