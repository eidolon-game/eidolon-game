#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EidolonAnimationBridge.generated.h"

UENUM(BlueprintType)
enum class EEidolonCinematicExit : uint8 { Completed, Skipped, Aborted };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEidolonCinematicExitEvent, FName, SceneId, EEidolonCinematicExit, Reason);

// Presentation only. Caller owns camera/input restoration and authoritative campaign progression.
UCLASS(ClassGroup=(Eidolon), meta=(BlueprintSpawnableComponent))
class EIDOLON_API UEidolonAnimationBridge : public UActorComponent
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Animation")
    bool BeginScene(FName SceneId, FGuid& Token)
    {
        Token = FGuid();
        if (SceneId.IsNone() || ActiveToken.IsValid()) return false;
        ActiveScene = SceneId;
        ActiveToken = FGuid::NewGuid();
        Token = ActiveToken;
        return true;
    }

    UFUNCTION(BlueprintCallable, Category="Animation")
    bool EndScene(FGuid Token, EEidolonCinematicExit Reason)
    {
        if (!ActiveToken.IsValid() || Token != ActiveToken) return false;
        const FName Scene = ActiveScene;
        ActiveToken.Invalidate();
        ActiveScene = NAME_None;
        OnSceneExit.Broadcast(Scene, Reason);
        return true;
    }

    UPROPERTY(BlueprintAssignable, Category="Animation")
    FEidolonCinematicExitEvent OnSceneExit;

    UFUNCTION(BlueprintPure, Category="Animation")
    bool IsSceneActive() const { return ActiveToken.IsValid(); }

protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override
    {
        if (ActiveToken.IsValid()) EndScene(ActiveToken, EEidolonCinematicExit::Aborted);
        Super::EndPlay(Reason);
    }
private:
    FGuid ActiveToken;
    FName ActiveScene;
};
