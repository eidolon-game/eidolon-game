#pragma once
#include "Combat/EidolonCombatRules.h"
namespace EidolonCombat
{
struct FProfileAttack { FAttack Attack; float Reach=220.f, Radius=45.f; };
inline FProfileAttack PlayerAttack(bool Heavy)
{
    FProfileAttack P;
    if(Heavy){ P.Attack.Damage=40; P.Attack.Stamina=28; P.Attack.Poise=45; P.Attack.Windup=.55f; P.Attack.Recovery=.65f; P.Reach=250; }
    return P;
}
enum class EEnemyPhase : uint8 { FirstMonster, CustodianBound, CustodianExposed, CustodianEnraged, Defeated };
inline EEnemyPhase CustodianPhase(float HealthFraction,int32 RemainingAnchors)
{
    if(HealthFraction<=0) return EEnemyPhase::Defeated;
    if(RemainingAnchors>0) return EEnemyPhase::CustodianBound;
    return HealthFraction<=.35f ? EEnemyPhase::CustodianEnraged : EEnemyPhase::CustodianExposed;
}
inline FProfileAttack EnemyAttack(EEnemyPhase Phase)
{
    FProfileAttack P; P.Attack.Stamina=8; P.Attack.Damage=12; P.Attack.Windup=.65f; P.Attack.Recovery=.85f; P.Reach=190;
    if(Phase==EEnemyPhase::CustodianBound){ P.Attack.Damage=24; P.Attack.Windup=1.1f; P.Reach=240; }
    if(Phase==EEnemyPhase::CustodianExposed){ P.Attack.Damage=28; P.Attack.Windup=.85f; P.Reach=250; }
    if(Phase==EEnemyPhase::CustodianEnraged){ P.Attack.Damage=32; P.Attack.Windup=.7f; P.Attack.bParryable=false; P.Attack.bBlockable=false; P.Reach=250; }
    return P;
}
}
