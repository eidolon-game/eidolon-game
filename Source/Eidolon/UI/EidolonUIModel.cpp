#include "EidolonUIModel.h"
#include "../EidolonCharacter.h"
#include "../EidolonCampaignSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"

FEidolonMeter FEidolonMeter::FromFraction(float Value)
{
    FEidolonMeter Meter;
    Meter.bAvailable = FMath::IsFinite(Value);
    Meter.Fraction = Meter.bAvailable ? FMath::Clamp(Value, 0.f, 1.f) : 0.f;
    return Meter;
}

void FEidolonAccessibilityOptions::Normalize()
{
    TextScale = FMath::IsFinite(TextScale) ? FMath::Clamp(TextScale, 1.f, 2.f) : 1.f;
}

FEidolonHUDSnapshot UEidolonUIModel::Capture(APlayerController* Controller)
{
    FEidolonHUDSnapshot Result;
    if (!IsValid(Controller)) return Result;
    if (const auto* Player = Cast<AEidolonCharacter>(Controller->GetPawn()))
    {
        Result.Health = FEidolonMeter::FromFraction(Player->GetHealthFraction());
        Result.Stamina = FEidolonMeter::FromFraction(Player->GetStaminaFraction());
        Result.bDefeated = Player->IsDefeated();
    }
    if (auto* GI = Controller->GetGameInstance())
        if (auto* Campaign = GI->GetSubsystem<UEidolonCampaignSubsystem>())
        {
            const auto Chapter = Campaign->CurrentChapter();
            Result.Chapter = Chapter.Id;
            Result.Level = Campaign->GetLevel();
            Result.Rank = Campaign->GetRank();
            Result.Objective = FText::FromString(Chapter.Objective);
            Result.ObjectiveTarget = FMath::Max(0, Chapter.Target);
            Result.ObjectiveCount = FMath::Clamp(Campaign->GetObjectiveCount(), 0, Result.ObjectiveTarget);
        }
    return Result;
}

void UEidolonUIModel::Reset(EEidolonScreen Screen)
{
    Stack.Reset();
    Stack.Add(Screen);
}

bool UEidolonUIModel::Push(EEidolonScreen Screen)
{
    if (Screen == GetScreen() || Stack.Num() >= 16) return false;
    if (Stack.IsEmpty()) Stack.Add(EEidolonScreen::Title);
    Stack.Add(Screen);
    return true;
}

bool UEidolonUIModel::Back()
{
    if (Stack.Num() <= 1) return false;
    Stack.Pop();
    return true;
}
