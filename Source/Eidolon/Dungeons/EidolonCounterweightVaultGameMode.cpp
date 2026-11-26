#include "EidolonCounterweightVaultGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "../Quest/EidolonFirstDescentInteraction.h"
#include "EidolonAbyssActorBindings.h"
#include "EidolonAbyssProgression.h"
#include "EidolonVaultPincer.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/PlayerStart.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace
{
    using EidolonPincerWinch::Signal;
    using EidolonPincerWinch::Stage;

    constexpr double RangeCm = 180.0;
    constexpr double PincerHeightCm = 50.0;
    constexpr double CrawlerHeightCm = 55.0;
    constexpr float EntranceWarningSeconds = 0.95f;
    const FName FloorPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor008"));
    const FString NextPackage(TEXT("/Game/Eidolon/Maps/AbyssFloor009"));
    const TCHAR* CheckpointOption = TEXT("AbyssCheckpoint");

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
            ? Italian : English;
    }

    bool CanUseFloor(const UEidolonCampaignSubsystem& Campaign)
    {
        return Campaign.CurrentChapter().Id == EidolonAbyss::Chapter && Campaign.GetEnding().IsNone()
            && Campaign.GetAuthoredAbyssCompletedFloor() >= EidolonAbyss::PincerWinchFloor - 1;
    }

    bool CanReach(const AEidolonCharacter& Player, const ATargetPoint* Target)
    {
        return IsValid(Target) && EidolonQuest::CanReachInteractionTarget(Player, *Target, RangeCm);
    }

    template <typename EnemyType>
    EnemyType* SpawnAt(UWorld& World, const ATargetPoint& Anchor, double Height)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        return World.SpawnActor<EnemyType>(Anchor.GetActorLocation() + FVector(0, 0, Height),
            Anchor.GetActorRotation(), Params);
    }
}

UEidolonCampaignSubsystem* AEidolonCounterweightVaultGameMode::GetCampaign() const
{
    auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

void AEidolonCounterweightVaultGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    bBound = BindVault();
    if (!bBound)
    {
        Notice = Display(TEXT("The vault layout is incomplete."), TEXT("La mappa della volta è incompleta."));
        return;
    }

    const auto* Campaign = GetCampaign();
    const bool bCanUseFloor = Campaign && CanUseFloor(*Campaign);
    const Stage Current = bCanUseFloor ? Campaign->GetAbyssPincerWinchStage() : Stage::Initial;
    UpdateVault(Current);
    if (bCanUseFloor && Current == Stage::FirstWinchEngaged) AnnounceCrawler();
}

bool AEidolonCounterweightVaultGameMode::BindVault()
{
    auto* World = GetWorld();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != TEXT("AbyssFloor008") ||
        FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
    {
        return false;
    }

    using EidolonAbyss::FindTaggedActor;
    Arena = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_008_encounter"));
    PincerSpawn = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_008_vault_pincer_01"));
    CrawlerSpawn = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_008_catacomb_crawler_01"));
    Winch = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_008_winch"));
    Exit = FindTaggedActor<ATargetPoint>(*World, TEXT("abyss_008_exit"));
    Gate = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_008_vault_gate"));
    FirstLatch = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_008_latch_01"));
    SecondLatch = FindTaggedActor<AStaticMeshActor>(*World, TEXT("abyss_008_latch_02"));
    return Arena.IsValid() && PincerSpawn.IsValid() && CrawlerSpawn.IsValid() && Winch.IsValid() &&
        Exit.IsValid() && Gate.IsValid() && FirstLatch.IsValid() && SecondLatch.IsValid()
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_008_player_start"))
        && FindTaggedActor<APlayerStart>(*World, TEXT("abyss_008_checkpoint_start"));
}

AActor* AEidolonCounterweightVaultGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* World = GetWorld())
    {
        const FName Tag = UGameplayStatics::HasOption(OptionsString, CheckpointOption)
            ? TEXT("abyss_008_checkpoint_start") : TEXT("abyss_008_player_start");
        if (auto* Start = EidolonAbyss::FindTaggedActor<APlayerStart>(*World, Tag)) return Start;
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

void AEidolonCounterweightVaultGameMode::Interact(AEidolonCharacter* Player)
{
    auto* Campaign = GetCampaign();
    if (!bBound || !Gate.IsValid() || !IsValid(Player) || Player->IsDefeated() ||
        Player != UGameplayStatics::GetPlayerCharacter(this, 0) || !Campaign || !CanUseFloor(*Campaign))
    {
        return;
    }

    const Stage Current = Campaign->GetAbyssPincerWinchStage();
    if (Current == Stage::Open)
    {
        if (!CanReach(*Player, Exit.Get())) return;
        if (FPackageName::DoesPackageExist(NextPackage))
            UGameplayStatics::OpenLevel(this, FName(*NextPackage));
        else
            Notice = Display(TEXT("The route below is unavailable."), TEXT("Il passaggio al piano inferiore non è disponibile."));
        return;
    }

    const bool bAtArena = CanReach(*Player, Arena.Get());
    const bool bAtWinch = CanReach(*Player, Winch.Get());
    if (PendingDefeat)
    {
        if (bAtArena || bAtWinch)
        {
            if (Commit(*Campaign, Current, *PendingDefeat)) PendingDefeat.reset();
        }
        return;
    }

    switch (Current)
    {
        case Stage::Initial:
            if (bAtArena && !IsValid(ActiveEnemy.Get())) SpawnPincer();
            break;
        case Stage::PincerDefeated:
            if (bAtWinch && Commit(*Campaign, Current, Signal::EngageFirstWinch)) AnnounceCrawler();
            break;
        case Stage::FirstWinchEngaged:
            if ((bAtWinch || bAtArena) && !IsValid(ActiveEnemy.Get()) &&
                !GetWorldTimerManager().IsTimerActive(CrawlerTimer))
            {
                AnnounceCrawler();
            }
            break;
        case Stage::CrawlerDefeated:
            if (bAtWinch) Commit(*Campaign, Current, Signal::EngageSecondWinch);
            break;
        case Stage::Open:
            break;
    }
}

void AEidolonCounterweightVaultGameMode::SpawnPincer()
{
    const auto* Anchor = PincerSpawn.Get();
    if (Anchor) ActiveEnemy = SpawnAt<AEidolonVaultPincer>(*GetWorld(), *Anchor, PincerHeightCm);
    if (IsValid(ActiveEnemy.Get()))
    {
        ActiveEnemy->OnEncounterDefeated.AddUObject(this, &AEidolonCounterweightVaultGameMode::OnEnemyDefeated);
        Notice.Reset();
    }
    else
    {
        Notice = Display(TEXT("Clear the arena entrance and interact to retry."),
                         TEXT("Libera l'ingresso dell'arena e interagisci per riprovare."));
    }
}

void AEidolonCounterweightVaultGameMode::AnnounceCrawler()
{
    Notice = Display(TEXT("Movement in the eastern passage."), TEXT("Qualcosa si muove nel passaggio orientale."));
    GetWorldTimerManager().SetTimer(CrawlerTimer, this, &AEidolonCounterweightVaultGameMode::SpawnCrawler,
        EntranceWarningSeconds, false);
}

void AEidolonCounterweightVaultGameMode::SpawnCrawler()
{
    const auto* Campaign = GetCampaign();
    if (!bBound || !Campaign || !CanUseFloor(*Campaign) ||
        Campaign->GetAbyssPincerWinchStage() != Stage::FirstWinchEngaged || IsValid(ActiveEnemy.Get()))
    {
        return;
    }

    const auto* Anchor = CrawlerSpawn.Get();
    if (Anchor) ActiveEnemy = SpawnAt<AEidolonVaultCrawler>(*GetWorld(), *Anchor, CrawlerHeightCm);
    if (IsValid(ActiveEnemy.Get()))
    {
        ActiveEnemy->OnEncounterDefeated.AddUObject(this, &AEidolonCounterweightVaultGameMode::OnEnemyDefeated);
        Notice.Reset();
    }
    else
    {
        Notice = Display(TEXT("Clear the eastern passage and interact at the winch to retry."),
                         TEXT("Libera il passaggio orientale e interagisci all'argano per riprovare."));
    }
}

void AEidolonCounterweightVaultGameMode::OnEnemyDefeated(AEidolonArenaEnemy& Enemy)
{
    auto* Campaign = GetCampaign();
    if (ActiveEnemy.Get() != &Enemy || !bBound || !Campaign) return;

    ActiveEnemy = nullptr;
    const Stage Current = Campaign->GetAbyssPincerWinchStage();
    if (Current == Stage::Initial && Enemy.IsA<AEidolonVaultPincer>())
        PendingDefeat = Signal::ConfirmPincerDefeat;
    else if (Current == Stage::FirstWinchEngaged && Enemy.IsA<AEidolonVaultCrawler>())
        PendingDefeat = Signal::ConfirmCrawlerDefeat;
    else
        return;

    if (Commit(*Campaign, Current, *PendingDefeat)) PendingDefeat.reset();
}

bool AEidolonCounterweightVaultGameMode::Commit(UEidolonCampaignSubsystem& Campaign, Stage Expected, Signal Verified)
{
    if (!Campaign.CommitAbyssPincerWinch(Verified, Expected))
    {
        Notice = Display(TEXT("Checkpoint failed. Interact to retry."),
                         TEXT("Salvataggio fallito. Interagisci per riprovare."));
        return false;
    }

    Notice.Reset();
    UpdateVault(Campaign.GetAbyssPincerWinchStage());
    return true;
}

void AEidolonCounterweightVaultGameMode::UpdateVault(Stage Current)
{
    const bool bOpen = Current == Stage::Open;
    const bool bFirstEngaged = Current == Stage::FirstWinchEngaged || Current == Stage::CrawlerDefeated || bOpen;
    if (auto* Door = Gate.Get())
    {
        Door->SetActorEnableCollision(!bOpen);
        Door->SetActorHiddenInGame(bOpen);
    }
    if (auto* Latch = FirstLatch.Get()) Latch->SetActorHiddenInGame(!bFirstEngaged);
    if (auto* Latch = SecondLatch.Get()) Latch->SetActorHiddenInGame(!bOpen);
}

void AEidolonCounterweightVaultGameMode::ReloadCheckpoint()
{
    if (!bBound) return;
    if (auto* Campaign = GetCampaign())
    {
        if (!Campaign->LoadCampaign())
            UE_LOG(LogTemp, Warning, TEXT("Vault checkpoint could not be read. Preserving current state."));
        if (CanUseFloor(*Campaign)) UGameplayStatics::OpenLevel(this, FloorPackage, true, CheckpointOption);
    }
}

FString AEidolonCounterweightVaultGameMode::GetPrompt() const
{
    if (!Notice.IsEmpty()) return Notice;
    const auto* Campaign = GetCampaign();
    if (!Campaign || !CanUseFloor(*Campaign))
        return Display(TEXT("Enter after completing the previous floor."), TEXT("Entra dopo aver completato il piano precedente."));

    switch (Campaign->GetAbyssPincerWinchStage())
    {
        case Stage::Initial:
            return IsValid(ActiveEnemy.Get())
                ? Display(TEXT("Circle the heavy claw."), TEXT("Aggira la chela pesante."))
                : Display(TEXT("Interact at the arena entrance."), TEXT("Interagisci all'ingresso dell'arena."));
        case Stage::PincerDefeated:
            return Display(TEXT("Engage the first winch latch."), TEXT("Inserisci il primo fermo dell'argano."));
        case Stage::FirstWinchEngaged:
            return Display(TEXT("Defeat the crawler from the eastern passage."), TEXT("Sconfiggi lo Strisciante del passaggio orientale."));
        case Stage::CrawlerDefeated:
            return Display(TEXT("Engage the second winch latch."), TEXT("Inserisci il secondo fermo dell'argano."));
        case Stage::Open:
            return Display(TEXT("Use the lower stair."), TEXT("Usa la scala verso il basso."));
    }
    return {};
}

FString AEidolonCounterweightVaultGameMode::GetNarrative() const
{
    return Display(TEXT("Counterweight Vault | Abyss, floor 8"), TEXT("Volta del contrappeso | Abisso, livello 8"));
}

void AEidolonVaultCrawler::BeginPlay()
{
    Super::BeginPlay();
    Configure(TEXT("abyss_catacomb_crawler"), 8);
}

float AEidolonVaultCrawler::TakeDamage(float Amount, const FDamageEvent& Event,
                                      AController* Instigator, AActor* Causer)
{
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!HasAuthority() || !Player || Player->IsDefeated() || Causer != Player || !Instigator ||
        Player->GetController() != Instigator || !FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return 0.0f;
    }
    return Super::TakeDamage(Amount, Event, Instigator, Causer);
}
