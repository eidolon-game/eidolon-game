#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EidolonPlayerSettings.generated.h"

// Separate slot from campaign saves. Local preferences only.
UCLASS()
class EIDOLON_API UEidolonPlayerSettings : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Input") float MouseSensitivity = 1.f;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Input") float StickSensitivity = 1.f;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Input") bool bInvertY = false;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Audio") float MasterVolume = 1.f;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Accessibility") bool bSubtitles = true;

    void Sanitize();
    UFUNCTION(BlueprintCallable, Category="Settings") static UEidolonPlayerSettings* LoadPreferences();
    UFUNCTION(BlueprintCallable, Category="Settings") bool SavePreferences();
private:
    UPROPERTY(SaveGame) int32 SchemaVersion = 1;
};
