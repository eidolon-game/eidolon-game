#pragma once
#include "../EidolonCharacter.h"
#include "EidolonM1Character.generated.h"

class UEidolonM1Adapter;
UCLASS()
class EIDOLON_API AEidolonM1Character : public AEidolonCharacter
{
    GENERATED_BODY()
public:
    AEidolonM1Character();
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UEidolonM1Adapter> RuntimeAdapter;
    FGuid DefeatEventId;
};
