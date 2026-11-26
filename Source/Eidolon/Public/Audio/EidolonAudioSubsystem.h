#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EidolonAudioSubsystem.generated.h"
class USoundBase;
class UAudioComponent;
UCLASS()
class EIDOLON_API UEidolonAudioSubsystem : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Eidolon|Audio")
 void SetContext(int32 Chapter, int32 Revision);
 UFUNCTION(BlueprintCallable, Category="Eidolon|Audio")
 bool PlayMusic(FName EventId, USoundBase* Sound, int32 MinChapter, int32 Revision, bool bAuthorized, float FadeSeconds);
 UFUNCTION(BlueprintCallable, Category="Eidolon|Audio")
 void SetIntensity(float Value);
 virtual void Deinitialize() override;
private:
 UPROPERTY(Transient) TObjectPtr<UAudioComponent> Music;
 FName ActiveId;
 int32 CurrentChapter = 1;
 int32 CurrentRevision = 0;
 float Intensity = 0;
 void StopMusic();
};
