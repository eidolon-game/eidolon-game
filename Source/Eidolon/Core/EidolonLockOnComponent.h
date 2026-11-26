#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EidolonLockOnComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEidolonTargetChanged, AActor*, Target);

UCLASS(ClassGroup=(Eidolon), meta=(BlueprintSpawnableComponent))
class EIDOLON_API UEidolonLockOnComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UEidolonLockOnComponent();
    UFUNCTION(BlueprintCallable, Category="Targeting") bool Acquire();
    UFUNCTION(BlueprintCallable, Category="Targeting") void Release();
    UFUNCTION(BlueprintPure, Category="Targeting") AActor* GetTarget() const;
    UPROPERTY(BlueprintAssignable, Category="Targeting") FEidolonTargetChanged OnTargetChanged;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditAnywhere, Category="Targeting", meta=(ClampMin="1")) float Range = 1800.f;
    UPROPERTY(EditAnywhere, Category="Targeting", meta=(ClampMin="-1", ClampMax="1")) float MinAlignment = 0.35f;
    UPROPERTY(EditAnywhere, Category="Targeting", meta=(ClampMin="0")) float RotationSpeed = 12.f;
private:
    TWeakObjectPtr<AActor> Target;
    bool bLocked = false;
    bool IsEligible(AActor* Actor, FVector Origin, FVector& Point) const;
};
