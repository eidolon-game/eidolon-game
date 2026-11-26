#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EidolonTargetable.generated.h"

UINTERFACE(BlueprintType)
class EIDOLON_API UEidolonTargetable : public UInterface
{
    GENERATED_BODY()
};
class EIDOLON_API IEidolonTargetable
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Targeting")
    bool CanBeTargeted() const;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Targeting")
    FVector GetTargetLocation() const;
};
