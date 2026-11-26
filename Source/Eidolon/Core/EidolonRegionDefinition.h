#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EidolonRegionDefinition.generated.h"

USTRUCT(BlueprintType)
struct FEidolonRegionRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RegionId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UWorld> Map;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RequiredCampaignFlag;
};

UCLASS(BlueprintType)
class EIDOLON_API UEidolonRegionDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World") FEidolonRegionRow Region;
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return Region.RegionId.IsNone() ? FPrimaryAssetId() : FPrimaryAssetId(FPrimaryAssetType(TEXT("EidolonRegion")), Region.RegionId);
    }
};
