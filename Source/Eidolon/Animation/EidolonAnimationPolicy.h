#pragma once
#include "CoreMinimal.h"
namespace EidolonAnimation
{
enum class EState : uint8 { Idle, Walk, Run, Sprint, Airborne, Land, Climb, Attack, Dodge, Block, Hit, Dead, Cinematic };
struct FSnapshot
{
    float SpeedCm = 0.f;
    bool bDead=false, bCinematic=false, bHit=false, bDodge=false, bAttack=false, bBlock=false;
    bool bClimb=false, bAirborne=false, bLanded=false, bSprint=false;
};
inline EState Resolve(const FSnapshot& S)
{
    if(S.bDead) return EState::Dead;
    if(S.bCinematic) return EState::Cinematic;
    if(S.bHit) return EState::Hit;
    if(S.bDodge) return EState::Dodge;
    if(S.bAttack) return EState::Attack;
    if(S.bBlock) return EState::Block;
    if(S.bClimb) return EState::Climb;
    if(S.bAirborne) return EState::Airborne;
    if(S.bLanded) return EState::Land;
    if(!FMath::IsFinite(S.SpeedCm) || S.SpeedCm < 5.f) return EState::Idle;
    if(S.bSprint) return EState::Sprint;
    return S.SpeedCm < 220.f ? EState::Walk : EState::Run;
}
// Cosmetic only; Combat owns damage, hit windows and resource spending.
enum class ENotify : uint8 { FootLeft, FootRight, Swing, ContactFX, Recover, Cloth };
struct FNotifyGate
{
    FGuid Action;
    TSet<int32> Seen;
    bool Begin(FGuid Id) { if(!Id.IsValid() || Action.IsValid()) return false; Action=Id; Seen.Reset(); return true; }
    bool Accept(FGuid Id, int32 CueIndex)
    { if(!Action.IsValid() || Id!=Action || CueIndex<0 || Seen.Contains(CueIndex)) return false; Seen.Add(CueIndex); return true; }
    bool End(FGuid Id) { if(!Action.IsValid() || Id!=Action) return false; Action.Invalidate(); Seen.Reset(); return true; }
};
}
