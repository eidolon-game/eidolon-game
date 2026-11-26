#include "EidolonArenaHUD.h"
#include "EidolonCharacter.h"
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
    DrawText(TEXT("EIDOLON | The First Descent - combat sandbox"), FLinearColor::White, 30, 24);
    DrawRect(FLinearColor(0.12f,0.12f,0.12f),30,60,250,18);
    DrawRect(FLinearColor(0.75f,0.12f,0.12f),30,60,250*Player->GetHealthFraction(),18);
    DrawRect(FLinearColor(0.12f,0.12f,0.12f),30,86,250,12);
    DrawRect(FLinearColor(0.2f,0.8f,0.35f),30,86,250*Player->GetStaminaFraction(),12);
    DrawText(FString::Printf(TEXT("Enemies: %d"), Remaining), FLinearColor::White,30,110);
    DrawText(TEXT("WASD / stick | LMB / RB light | RMB / RT heavy | Q / B dodge | F / LB block"), FLinearColor::White,30,Canvas->ClipY-55);
    if (Player->IsDefeated() || Remaining == 0)
        DrawText(Player->IsDefeated() ? TEXT("DEFEATED - R / Y to restart") : TEXT("ARENA CLEARED - R / Y to restart"), FLinearColor::Yellow,30,150,nullptr,1.5f);
}
