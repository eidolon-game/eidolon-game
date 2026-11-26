#include "EidolonAbyssFloorGameMode.h"

#include "EidolonCampaignSubsystem.h"
#include "EidolonCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace
{
    const FName AbyssMap(TEXT("/Game/Eidolon/Maps/AbyssFloor001"));
    const FString SurfaceMap(TEXT("/Game/Eidolon/Maps/ValdrisPrologue"));
    constexpr int32 ArrivalStep = 1;
    constexpr int32 EncounterStep = 2;
    constexpr int32 AnomalyStep = 3;
    constexpr int32 SurfaceStep = 4;
    constexpr double InteractionDistance = 180.0;

    const TCHAR* FloorText(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName()
            == TEXT("it") ? Italian : English;
    }

    template <typename ActorType>
    ActorType* FindFloorActor(UWorld& World, FName Tag)
    {
        ActorType* Found = nullptr;
        for (TActorIterator<AActor> It(&World); It; ++It)
        {
            if (It->IsActorBeingDestroyed() || !It->ActorHasTag(Tag)) continue;
            if (Found) return nullptr;
            Found = Cast<ActorType>(*It);
            if (!Found) return nullptr;
        }
        return Found;
    }

    EidolonFirstDescent::Event MakeEvent(int32 Step)
    {
        const auto Index = static_cast<std::size_t>(Step);
        return {2, "mq_02", Index, EidolonFirstDescent::Steps[Index]};
    }
}

void AEidolonAbyssFloorGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    bFloorBound = BindFloor();
    if (!bFloorBound)
    {
        FloorNotice = FloorText(TEXT("First Descent layout is incomplete."), TEXT("La Prima Discesa è incompleta."));
        UE_LOG(LogTemp, Error, TEXT("AbyssFloor001 is missing a unique native encounter binding."));
        return;
    }

    auto* Campaign = GetCampaign();
    if (!Campaign) return;
    if (Campaign->CurrentChapter().Id == 3)
    {
        UpdateGate(AnomalyStep);
        return;
    }
    if (Campaign->CurrentChapter().Id != 2) return;
    const int32 Cursor = Campaign->GetFirstDescentCursor();
    UpdateGate(Cursor);
    auto* Player = Cast<AEidolonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (Cursor == ArrivalStep && IsActivePlayer(Player))
    {
        ArriveAtFloor(*Campaign, *Player);
        return;
    }
    if (Cursor >= EncounterStep && Cursor <= SurfaceStep && IsActivePlayer(Player))
    {
        const FVector Respawn = Checkpoint->GetActorLocation() + FVector(0, 0, 110);
        Player->TeleportTo(Respawn, Checkpoint->GetActorRotation());
    }
    if (Cursor == EncounterStep) SpawnNextCrawler();
}

bool AEidolonAbyssFloorGameMode::BindFloor()
{
    UWorld* World = GetWorld();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != TEXT("AbyssFloor001")
        || FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
    {
        return false;
    }

    Entry = FindFloorActor<ATargetPoint>(*World, TEXT("abyss_001_entry"));
    Checkpoint = FindFloorActor<ATargetPoint>(*World, TEXT("checkpoint_abyss_001"));
    Anomaly = FindFloorActor<ATargetPoint>(*World, TEXT("first_anomaly"));
    SurfaceExit = FindFloorActor<ATargetPoint>(*World, TEXT("surface_exit"));
    Riven = FindFloorActor<ATargetPoint>(*World, TEXT("abyss_001_riven"));
    CrawlerSpawns[0] = FindFloorActor<ATargetPoint>(*World, TEXT("abyss_001_crawler_01"));
    CrawlerSpawns[1] = FindFloorActor<ATargetPoint>(*World, TEXT("abyss_001_crawler_02"));
    Gate = FindFloorActor<AStaticMeshActor>(*World, TEXT("abyss_001_combat_gate"));

    return Entry.IsValid() && Checkpoint.IsValid() && Anomaly.IsValid() && SurfaceExit.IsValid()
        && CrawlerSpawns[0].IsValid() && CrawlerSpawns[1].IsValid() && Gate.IsValid()
        && Interactions.BindTarget(ArrivalStep, "abyss_001", *Entry.Get())
        && Interactions.BindTarget(AnomalyStep, "anomaly", *Anomaly.Get())
        && Interactions.BindTarget(SurfaceStep, "valdris_abyss", *SurfaceExit.Get());
}

UEidolonCampaignSubsystem* AEidolonAbyssFloorGameMode::GetCampaign() const
{
    UGameInstance* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

bool AEidolonAbyssFloorGameMode::IsActivePlayer(const AEidolonCharacter* Player) const
{
    const UWorld* World = GetWorld();
    const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    return IsValid(Player) && Player->GetWorld() == World && !Player->IsDefeated()
        && Controller && Controller->GetPawn() == Player;
}

bool AEidolonAbyssFloorGameMode::CommitStep(UEidolonCampaignSubsystem& Campaign, int32 Step)
{
    if (!Campaign.CommitFirstDescentEvent(MakeEvent(Step)))
    {
        FloorNotice = FloorText(TEXT("Progress could not be saved. Interact to retry."),
                               TEXT("Impossibile salvare i progressi. Interagisci per riprovare."));
        return false;
    }
    FloorNotice.Reset();
    UpdateGate(Campaign.GetFirstDescentCursor());
    return true;
}

void AEidolonAbyssFloorGameMode::ArriveAtFloor(UEidolonCampaignSubsystem& Campaign, const AEidolonCharacter& Player)
{
    const ATargetPoint* Marker = Entry.Get();
    if (Marker && Interactions.CanInteract(Player, *Marker, ArrivalStep, MakeEvent(ArrivalStep), InteractionDistance)
        && CommitStep(Campaign, ArrivalStep))
    {
        SpawnNextCrawler();
    }
}

void AEidolonAbyssFloorGameMode::SpawnNextCrawler()
{
    if (IsValid(ActiveCrawler.Get()) || DefeatedCrawlers >= CrawlerCount) return;
    ATargetPoint* Spawn = CrawlerSpawns[DefeatedCrawlers].Get();
    if (!Spawn) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    ActiveCrawler = GetWorld()->SpawnActor<AEidolonFirstDescentCrawler>(
        Spawn->GetActorLocation() + FVector(0, 0, 55), Spawn->GetActorRotation(), Params);
    if (IsValid(ActiveCrawler.Get()))
    {
        ActiveCrawler->Configure(TEXT("first_descent_crawler"), 1);
        FloorNotice.Reset();
        return;
    }
    FloorNotice = FloorText(TEXT("Crawler could not spawn. Clear the spawn point and interact."),
                            TEXT("Il mostro non è comparso. Libera il punto di comparsa e interagisci."));
}

void AEidolonAbyssFloorGameMode::ConfirmCrawlerDefeat(AEidolonFirstDescentCrawler& Crawler)
{
    auto* Campaign = GetCampaign();
    if (ActiveCrawler.Get() != &Crawler || !Campaign || Campaign->GetFirstDescentCursor() != EncounterStep) return;
    ActiveCrawler = nullptr;
    ++DefeatedCrawlers;
    if (DefeatedCrawlers == CrawlerCount)
    {
        CommitStep(*Campaign, EncounterStep);
    }
    else
    {
        SpawnNextCrawler();
    }
}

void AEidolonAbyssFloorGameMode::UpdateGate(int32 Cursor)
{
    if (auto* Door = Gate.Get())
    {
        const bool bOpen = Cursor >= AnomalyStep;
        Door->SetActorEnableCollision(!bOpen);
        Door->SetActorHiddenInGame(bOpen);
    }
}

void AEidolonAbyssFloorGameMode::Interact(AEidolonCharacter* Player)
{
    auto* Campaign = GetCampaign();
    if (!bFloorBound || !Gate.IsValid() || !IsActivePlayer(Player) || !Campaign
        || !Campaign->GetEnding().IsNone())
    {
        return;
    }
    if (Campaign->CurrentChapter().Id == 3)
    {
        InteractOnSecondDay(*Campaign, *Player);
        return;
    }
    if (Campaign->CurrentChapter().Id != 2) return;

    switch (Campaign->GetFirstDescentCursor())
    {
        case ArrivalStep:
            ArriveAtFloor(*Campaign, *Player);
            break;
        case EncounterStep:
            if (DefeatedCrawlers == CrawlerCount)
                CommitStep(*Campaign, EncounterStep);
            else
                SpawnNextCrawler();
            break;
        case AnomalyStep:
            if (const ATargetPoint* Marker = Anomaly.Get();
                Marker && Interactions.CanInteract(*Player, *Marker, AnomalyStep, MakeEvent(AnomalyStep), InteractionDistance))
            {
                if (!bAnomalyObserved)
                    bAnomalyObserved = true;
                else if (CommitStep(*Campaign, AnomalyStep))
                    bAnomalyObserved = false;
            }
            break;
        case SurfaceStep:
            if (const ATargetPoint* Marker = SurfaceExit.Get();
                Marker && Interactions.CanReachTarget(*Player, *Marker, SurfaceStep, InteractionDistance))
            {
                if (!FPackageName::DoesPackageExist(SurfaceMap) || !Campaign->RequestFirstDescentSurfaceReturn())
                {
                    FloorNotice = FloorText(TEXT("The route to Valdris is unavailable."),
                                            TEXT("Il passaggio per Valdris non è disponibile."));
                    return;
                }
                UGameplayStatics::OpenLevel(this, FName(*SurfaceMap));
            }
            break;
        default:
            break;
    }
}

void AEidolonAbyssFloorGameMode::InteractOnSecondDay(UEidolonCampaignSubsystem& Campaign, const AEidolonCharacter& Player)
{
    if (Campaign.HasFlag(TEXT("chapter_3_riven_spoken")))
    {
        const auto* Exit = SurfaceExit.Get();
        if (Exit && EidolonQuest::CanReachInteractionTarget(Player, *Exit, InteractionDistance) &&
            FPackageName::DoesPackageExist(SurfaceMap))
        {
            UGameplayStatics::OpenLevel(this, FName(*SurfaceMap));
        }
        return;
    }

    const auto* Companion = Riven.Get();
    if (!Companion)
    {
        FloorNotice = FloorText(TEXT("Riven's scene marker is missing."),
                               TEXT("Il punto della scena di Riven è mancante."));
        return;
    }
    if (!EidolonQuest::CanReachInteractionTarget(Player, *Companion, InteractionDistance)) return;
    const auto Chapter = Campaign.CurrentChapter();
    if (Chapter.Dialogue.IsValidIndex(RivenLine))
    {
        RivenDialogue = Chapter.Dialogue[RivenLine++];
        return;
    }
    if (!Campaign.CommitRivenConversation())
    {
        FloorNotice = FloorText(TEXT("Progress could not be saved. Interact to retry."),
                               TEXT("Impossibile salvare i progressi. Interagisci per riprovare."));
        return;
    }
    FloorNotice.Reset();
}

void AEidolonAbyssFloorGameMode::ReloadCheckpoint()
{
    if (auto* Campaign = GetCampaign())
    {
        if (!Campaign->LoadCampaign())
        {
            UE_LOG(LogTemp, Warning, TEXT("Abyss floor checkpoint could not be read. Preserving the current campaign state."));
        }
        UGameplayStatics::OpenLevel(this, AbyssMap);
    }
}

FString AEidolonAbyssFloorGameMode::GetPrompt() const
{
    if (!FloorNotice.IsEmpty()) return FloorNotice;
    const auto* Campaign = GetCampaign();
    if (Campaign && Campaign->CurrentChapter().Id == 3)
    {
        return Campaign->HasFlag(TEXT("chapter_3_riven_spoken"))
            ? FloorText(TEXT("Return to the surface."), TEXT("Torna in superficie."))
            : FloorText(TEXT("Speak to Riven past the checkpoint."), TEXT("Parla con Riven oltre il checkpoint."));
    }
    if (!Campaign || Campaign->CurrentChapter().Id != 2)
        return FloorText(TEXT("Start the descent at the Guild."), TEXT("Inizia la discesa dalla Gilda."));

    switch (Campaign->GetFirstDescentCursor())
    {
        case ArrivalStep:
            return FloorText(TEXT("E / X or Square at the entrance."), TEXT("E / X o Quadrato all'ingresso."));
        case EncounterStep:
            return FloorText(TEXT("Defeat the crawlers."), TEXT("Sconfiggi i mostri."));
        case AnomalyStep:
            return bAnomalyObserved
                ? FloorText(TEXT("E / X or Square to continue."), TEXT("E / X o Quadrato per proseguire."))
                : FloorText(TEXT("Investigate beyond the gate. E / X or Square."),
                            TEXT("Esamina la sala oltre il cancello. E / X o Quadrato."));
        case SurfaceStep:
            return FloorText(TEXT("E / X or Square at the surface exit."), TEXT("E / X o Quadrato all'uscita verso la superficie."));
        default:
            return FloorText(TEXT("Start the descent at the Guild."), TEXT("Inizia la discesa dalla Gilda."));
    }
}

FString AEidolonAbyssFloorGameMode::GetNarrative() const
{
    if (const auto* Campaign = GetCampaign(); Campaign && Campaign->CurrentChapter().Id == 3)
        return FloorText(TEXT("The Weight of Memories | Abyss, floor 1"),
                         TEXT("Il peso dei ricordi | Abisso, livello 1"));
    return FloorText(TEXT("First Descent | Abyss, floor 1"), TEXT("Prima Discesa | Abisso, livello 1"));
}

FString AEidolonAbyssFloorGameMode::GetDialogue() const
{
    if (!RivenDialogue.IsEmpty()) return RivenDialogue;
    return bAnomalyObserved
        ? FloorText(TEXT("[The chamber warps. A muffled voice echoes briefly.]"),
                    TEXT("[La sala si deforma. Una voce indistinta risuona per un istante.]"))
        : TEXT("");
}

float AEidolonFirstDescentCrawler::TakeDamage(float Amount, const FDamageEvent& Event,
                                             AController* Instigator, AActor* Causer)
{
    auto* Mode = GetWorld()->GetAuthGameMode<AEidolonAbyssFloorGameMode>();
    auto* Player = Cast<AEidolonCharacter>(Causer);
    if (!FMath::IsFinite(Amount) || Amount <= 0.0f || !Mode || Mode->ActiveCrawler.Get() != this || !Mode->IsActivePlayer(Player)
        || !Instigator || Player->GetController() != Instigator)
    {
        return 0.0f;
    }

    const float Applied = Super::TakeDamage(Amount, Event, Instigator, Causer);
    if (Applied > 0.0f && GetHealth() <= 0.0f) Mode->ConfirmCrawlerDefeat(*this);
    return Applied;
}
