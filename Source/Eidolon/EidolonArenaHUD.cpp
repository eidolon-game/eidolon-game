#include "EidolonArenaHUD.h"
#include "EidolonCharacter.h"
#include "EidolonGameMode.h"
#include "EidolonCampaignSubsystem.h"
#include "Engine/GameInstance.h"
#include "EidolonArenaEnemy.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

void AEidolonArenaHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Canvas || !Player) return;
    int32 Remaining = 0;
    for (TActorIterator<AEidolonArenaEnemy> It(GetWorld()); It; ++It)
        if (!It->IsActorBeingDestroyed()) ++Remaining;
    DrawText(TEXT("EIDOLON | Campaign graybox"), FLinearColor::White, 30, 24);
    DrawRect(FLinearColor(0.12f,0.12f,0.12f),30,60,250,18);
    DrawRect(FLinearColor(0.75f,0.12f,0.12f),30,60,250*Player->GetHealthFraction(),18);
    DrawRect(FLinearColor(0.12f,0.12f,0.12f),30,86,250,12);
    DrawRect(FLinearColor(0.2f,0.8f,0.35f),30,86,250*Player->GetStaminaFraction(),12);
    DrawText(FString::Printf(TEXT("Enemies: %d"), Remaining), FLinearColor::White,30,110);
    DrawText(TEXT("WASD / stick | LMB / RB light | RMB / RT heavy | Q / B dodge | F / LB block"), FLinearColor::White,30,Canvas->ClipY-55);
    if (auto* GM = GetWorld()->GetAuthGameMode<AEidolonGameMode>())
    {
        DrawText(GM->GetNarrative(), FLinearColor::White,30,145);
        DrawText(GM->GetPrompt(), FLinearColor::Yellow,30,205);
    }
    if (auto* GI = GetWorld()->GetGameInstance())
        if (auto* C = GI->GetSubsystem<UEidolonCampaignSubsystem>())
        {
            const auto Chapter = C->CurrentChapter();
            DrawText(Chapter.Objective, FLinearColor::White,30,170);
            DrawText(FString::Printf(TEXT("Rank %s | Level %d | Objective %d/%d"), *C->GetRank().ToString(), C->GetLevel(), C->GetObjectiveCount(), Chapter.Target), FLinearColor::White,30,235);
            if (Chapter.Id == 4 || Chapter.Id == 23)
                DrawText(FString::Printf(TEXT("Floor %d/100"), C->GetFloor(Chapter.Id == 23)), FLinearColor::White,30,260);
        }
    if (Player->IsDefeated()) DrawText(TEXT("DEFEATED - R / Y to reload checkpoint"), FLinearColor::Red,30,290,nullptr,1.5f);
}
