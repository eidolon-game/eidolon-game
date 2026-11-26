#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "EidolonCoreGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEidolonRegionChanged, FName, Region);

UCLASS()
class EIDOLON_API AEidolonCoreGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="World") FName GetRegion() const { return Region; }
    UFUNCTION(BlueprintCallable, Category="World", BlueprintAuthorityOnly) bool SetRegion(FName NewRegion);
    UPROPERTY(BlueprintAssignable, Category="World") FEidolonRegionChanged OnRegionChanged;
private:
    UPROPERTY(Transient) FName Region;
};
