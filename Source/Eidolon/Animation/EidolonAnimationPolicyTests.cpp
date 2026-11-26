#include "Animation/EidolonAnimationPolicy.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEidolonAnimationPolicyTest,"Eidolon.Animation.Policy",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEidolonAnimationPolicyTest::RunTest(const FString&)
{
    using namespace EidolonAnimation;
    FSnapshot S; S.SpeedCm=200;
    TestTrue(TEXT("Walk"),Resolve(S)==EState::Walk);
    S.SpeedCm=300; TestTrue(TEXT("Run"),Resolve(S)==EState::Run);
    S.bAirborne=true; TestTrue(TEXT("Airborne"),Resolve(S)==EState::Airborne);
    S.bAttack=true; TestTrue(TEXT("Attack"),Resolve(S)==EState::Attack);
    S.bCinematic=true; TestTrue(TEXT("Cinematic"),Resolve(S)==EState::Cinematic);
    S.bDead=true; TestTrue(TEXT("Death priority"),Resolve(S)==EState::Dead);
    FNotifyGate G; const FGuid A=FGuid::NewGuid(), B=FGuid::NewGuid();
    TestTrue(TEXT("Begin"),G.Begin(A));
    TestFalse(TEXT("Overlap"),G.Begin(B));
    TestFalse(TEXT("Wrong token"),G.Accept(B,0));
    TestTrue(TEXT("First cue"),G.Accept(A,0));
    TestFalse(TEXT("Duplicate"),G.Accept(A,0));
    TestFalse(TEXT("Negative cue"),G.Accept(A,-1));
    TestTrue(TEXT("End"),G.End(A));
    TestFalse(TEXT("Late"),G.Accept(A,1));
    TestTrue(TEXT("New action"),G.Begin(B));
    TestTrue(TEXT("Reuse index"),G.Accept(B,0));
    return true;
}
#endif
