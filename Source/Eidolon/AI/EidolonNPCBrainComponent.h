#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EidolonNPCBrainComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEidolonNPCIntentChanged, FName, Intent);

UCLASS(ClassGroup=(Eidolon), meta=(BlueprintSpawnableComponent))
class EIDOLON_API UEidolonNPCBrainComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName NPCId;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName Role = TEXT("civilian");
 UPROPERTY(BlueprintAssignable, Category="NPC") FEidolonNPCIntentChanged OnIntentChanged;
 UFUNCTION(BlueprintCallable, Category="NPC")
 FName Evaluate(bool bRecruited, FName Order, bool bHostile, bool bSeen, bool bHeard, bool bAllyThreatened, bool bAllyCritical, bool bReinforcement, bool bInRange, bool bCanAct, bool bWork, bool bSocial, int32 HealthPercent);
private:
 FName PreviousIntent;
};
