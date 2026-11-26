#pragma once
#include "CoreMinimal.h"
#include "EidolonSubtitleModel.generated.h"

USTRUCT(BlueprintType)
struct FEidolonSubtitleCue
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Speaker;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Line;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSoundCue = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Duration = 3.f;
};

// Presentation only. Narrative owns cue disclosure and playback timing.
UCLASS()
class EIDOLON_API UEidolonSubtitleModel : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Eidolon|UI")
    bool Present(const FEidolonSubtitleCue& Cue, bool bEnabled)
    {
        Clear();
        if (!bEnabled || Cue.Id.IsNone() || Cue.Line.IsEmpty() ||
            !FMath::IsFinite(Cue.Duration) || Cue.Duration <= 0.f) return false;
        Active = Cue;
        Remaining = Cue.Duration;
        return true;
    }
    UFUNCTION(BlueprintCallable, Category="Eidolon|UI")
    void Advance(float DeltaSeconds)
    {
        if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.f) return;
        Remaining = FMath::Max(0.f, Remaining - DeltaSeconds);
        if (Remaining <= 0.f) Clear();
    }
    UFUNCTION(BlueprintCallable, Category="Eidolon|UI")
    void Clear() { Active = FEidolonSubtitleCue(); Remaining = 0.f; }
    UFUNCTION(BlueprintPure, Category="Eidolon|UI")
    bool IsVisible() const { return Remaining > 0.f; }
    UFUNCTION(BlueprintPure, Category="Eidolon|UI")
    FEidolonSubtitleCue GetActive() const { return Active; }
private:
    UPROPERTY() FEidolonSubtitleCue Active;
    float Remaining = 0.f;
};
