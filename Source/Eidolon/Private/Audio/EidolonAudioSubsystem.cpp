#include "Audio/EidolonAudioSubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
void UEidolonAudioSubsystem::StopMusic()
{
 if (IsValid(Music)) Music->Stop();
 Music = nullptr;
 ActiveId = NAME_None;
}
void UEidolonAudioSubsystem::SetContext(int32 Chapter, int32 Revision)
{
 if (Chapter < 1 || Chapter > 26 || Revision < 0) return;
 if (Revision != CurrentRevision || Chapter != CurrentChapter) StopMusic();
 CurrentChapter = Chapter;
 CurrentRevision = Revision;
}
bool UEidolonAudioSubsystem::PlayMusic(FName EventId, USoundBase* Sound, int32 MinChapter, int32 Revision, bool bAuthorized, float FadeSeconds)
{
 if (!bAuthorized || EventId.IsNone() || !IsValid(Sound) || !GetWorld() || MinChapter < 1 || MinChapter > CurrentChapter || Revision != CurrentRevision || !FMath::IsFinite(FadeSeconds)) return false;
 if (ActiveId == EventId && IsValid(Music) && Music->IsPlaying()) return true;
 const float Fade = FMath::Clamp(FadeSeconds, 0.f, 10.f);
 UAudioComponent* Next = UGameplayStatics::SpawnSound2D(this, Sound, 0.f, 1.f, 0.f, nullptr, false, true);
 if (!IsValid(Next)) return false;
 if (IsValid(Music)) Music->FadeOut(Fade, 0.f);
 Music = Next;
 ActiveId = EventId;
 Music->SetFloatParameter(FName(TEXT("Intensity")), Intensity);
 Music->FadeIn(Fade, 1.f);
 return true;
}
void UEidolonAudioSubsystem::SetIntensity(float Value)
{
 if (!FMath::IsFinite(Value)) return;
 Intensity = FMath::Clamp(Value, 0.f, 1.f);
 if (IsValid(Music)) Music->SetFloatParameter(FName(TEXT("Intensity")), Intensity);
}
void UEidolonAudioSubsystem::Deinitialize()
{
 StopMusic();
 Super::Deinitialize();
}
