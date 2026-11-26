#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EidolonMineBorerPresentation.generated.h"

class UStaticMesh;

UCLASS()
class EIDOLON_API UEidolonMineBorerPresentation : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category="Mine Borer") TObjectPtr<UStaticMesh> Body;
    UPROPERTY(EditAnywhere, Category="Mine Borer") TObjectPtr<UStaticMesh> Head;
    UPROPERTY(EditAnywhere, Category="Mine Borer") TObjectPtr<UStaticMesh> RearLeft;
    UPROPERTY(EditAnywhere, Category="Mine Borer") TObjectPtr<UStaticMesh> RearRight;

    UPROPERTY(EditAnywhere, Category="Mine Borer") FVector BodyPivot = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Mine Borer") FVector HeadPivot = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Mine Borer") FVector RearLeftPivot = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Mine Borer") FVector RearRightPivot = FVector::ZeroVector;

    // Each signed number selects the imported axis for canonical X, Y and Z.
    UPROPERTY(EditAnywhere, Category="Mine Borer") FIntVector ImportedAxes = FIntVector(1, 2, 3);
    UPROPERTY(EditAnywhere, Category="Mine Borer") double ImportScale = 1.0;
    UPROPERTY(EditAnywhere, Category="Mine Borer") FVector RearHingeAxis = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Mine Borer") double RearOpenRadians = 0.0;
};
