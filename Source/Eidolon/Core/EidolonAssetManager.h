#pragma once
#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "EidolonAssetManager.generated.h"

UCLASS()
class EIDOLON_API UEidolonAssetManager : public UAssetManager
{
    GENERATED_BODY()
public:
    virtual void StartInitialLoading() override;
};
