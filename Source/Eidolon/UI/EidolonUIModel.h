#pragma once
#include "CoreMinimal.h"
#include "EidolonUIModel.generated.h"

UENUM(BlueprintType)
enum class EEidolonScreen : uint8
{
    Title, NewGame, Load, Settings, Pause, Inventory, Equipment, Skills, Map, Journal,
    Dialogue, Merchant, Crafting, Memories, Companions, Tutorial, Death, Ending, Credits, HUD
};

USTRUCT(BlueprintType)
struct FEidolonMeter
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly) float Fraction = 0;
    static FEidolonMeter FromFraction(float Value);
};

USTRUCT(BlueprintType)
struct FEidolonHUDSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FEidolonMeter Health;
    UPROPERTY(BlueprintReadOnly) FEidolonMeter Stamina;
    UPROPERTY(BlueprintReadOnly) FEidolonMeter Mana;
    UPROPERTY(BlueprintReadOnly) int32 Chapter = 0;
    UPROPERTY(BlueprintReadOnly) int32 Level = 0;
    UPROPERTY(BlueprintReadOnly) FName Rank;
    UPROPERTY(BlueprintReadOnly) FText Objective;
    UPROPERTY(BlueprintReadOnly) int32 ObjectiveCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 ObjectiveTarget = 0;
    UPROPERTY(BlueprintReadOnly) bool bDefeated = false;
};

USTRUCT(BlueprintType)
struct FEidolonAccessibilityOptions
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TextScale = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSubtitles = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHighContrast = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bReducedMotion = false;
    void Normalize();
};

UCLASS()
class EIDOLON_API UEidolonUIModel : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Eidolon|UI") static FEidolonHUDSnapshot Capture(APlayerController* Controller);
    UFUNCTION(BlueprintPure, Category="Eidolon|UI") EEidolonScreen GetScreen() const { return Stack.IsEmpty() ? EEidolonScreen::Title : Stack.Last(); }
    UFUNCTION(BlueprintCallable, Category="Eidolon|UI") void Reset(EEidolonScreen Screen);
    UFUNCTION(BlueprintCallable, Category="Eidolon|UI") bool Push(EEidolonScreen Screen);
    UFUNCTION(BlueprintCallable, Category="Eidolon|UI") bool Back();
private:
    UPROPERTY() TArray<EEidolonScreen> Stack;
};
