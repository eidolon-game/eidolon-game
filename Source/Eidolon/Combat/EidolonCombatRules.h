#pragma once
#include "CoreMinimal.h"

namespace EidolonCombat
{
enum class EAction : uint8 { Light, Heavy, Charged, Dodge, Block, Counter, Ability };
enum class EDamage : uint8 { Physical, Magic, Fire, Frost, Lightning, Count };
enum class EOutcome : uint8 { Ignored, Evaded, Parried, Blocked, Hit, GuardBroken };

struct FAttack
{
    float Damage = 20.f, Stamina = 12.f, Mana = 0.f, Poise = 20.f;
    float Windup = .2f, Active = .12f, Recovery = .3f;
    EDamage Type = EDamage::Physical;
    bool bParryable = true, bBlockable = true;
};
struct FState
{
    float Health = 100.f, Stamina = 100.f, Mana = 100.f, Poise = 100.f;
    float MaxHealth = 100.f, MaxStamina = 100.f, MaxMana = 100.f, MaxPoise = 100.f;
    float ReadyAt = 0.f, DodgeUntil = 0.f, ParryUntil = 0.f, CounterUntil = 0.f, StaggerUntil = 0.f;
    float Resistances[static_cast<int32>(EDamage::Count)] = {};
    bool bBlocking = false;
    bool IsAlive() const { return Health > 0.f; }
    bool CanAct(float Now) const { return IsAlive() && Now >= ReadyAt && Now >= StaggerUntil && !bBlocking; }
    bool Spend(float S, float M)
    {
        if (!FMath::IsFinite(S) || !FMath::IsFinite(M) || S < 0 || M < 0 || Stamina < S || Mana < M) return false;
        Stamina -= S; Mana -= M; return true;
    }
    bool BeginAttack(const FAttack& A, float Now)
    {
        if (!CanAct(Now) || !FMath::IsFinite(Now) || !FMath::IsFinite(A.Damage) || A.Damage<=0 || !FMath::IsFinite(A.Poise) || A.Poise<0 || !FMath::IsFinite(A.Windup+A.Active+A.Recovery) || A.Windup < 0 || A.Active <= 0 || A.Recovery < 0 || !Spend(A.Stamina,A.Mana)) return false;
        ReadyAt = Now+A.Windup+A.Active+A.Recovery; return true;
    }
    bool Dodge(float Now)
    {
        if (!CanAct(Now) || !Spend(25.f,0.f)) return false;
        DodgeUntil=Now+.22f; ReadyAt=Now+.55f; return true;
    }
    bool BeginBlock(float Now)
    {
        if (!CanAct(Now)) return false;
        bBlocking=true; ParryUntil=Now+.15f; return true;
    }
    void EndBlock() { bBlocking=false; ParryUntil=0.f; }
    EOutcome Receive(const FAttack& A, float Now, bool bFront)
    {
        const int32 Index=static_cast<int32>(A.Type);
        if (!IsAlive() || Index>=static_cast<int32>(EDamage::Count) || !FMath::IsFinite(A.Damage) || A.Damage<=0 || !FMath::IsFinite(A.Poise) || A.Poise<0) return EOutcome::Ignored;
        if (Now<DodgeUntil) return EOutcome::Evaded;
        if (bBlocking && bFront && A.bParryable && Now<ParryUntil)
        { ParryUntil=0.f; CounterUntil=Now+.65f; return EOutcome::Parried; }
        float Damage=A.Damage*(1.f-FMath::Clamp(Resistances[Index],-.5f,.85f));
        EOutcome Result=EOutcome::Hit;
        if (bBlocking && bFront && A.bBlockable)
        {
            if (Spend(A.Damage*.4f,0.f)) { Damage*=.2f; Result=EOutcome::Blocked; }
            else { Stamina=0; EndBlock(); StaggerUntil=Now+.9f; Result=EOutcome::GuardBroken; }
        }
        Health=FMath::Max(0.f,Health-Damage);
        Poise=FMath::Max(0.f,Poise-A.Poise*(Result==EOutcome::Blocked?.2f:1.f));
        if (Poise<=0.f) { StaggerUntil=FMath::Max(StaggerUntil,Now+.65f); EndBlock(); Poise=MaxPoise; }
        if (!IsAlive()) EndBlock();
        return Result;
    }
    bool Counter(const FAttack& A,float Now)
    {
        if (Now>=CounterUntil || !IsAlive() || Now<ReadyAt || Now<StaggerUntil) return false;
        const bool WasBlocking=bBlocking;
        bBlocking=false;
        if (!BeginAttack(A,Now)) { bBlocking=WasBlocking; return false; }
        EndBlock();
        CounterUntil=0.f; return true;
    }
    void Recover(float Delta,float Now)
    {
        if (!IsAlive() || !FMath::IsFinite(Delta) || Delta<=0) return;
        if (!bBlocking && Now>=ReadyAt && Now>=StaggerUntil) Stamina=FMath::Min(MaxStamina,Stamina+18.f*Delta);
        Mana=FMath::Min(MaxMana,Mana+3.f*Delta);
    }
};
}
