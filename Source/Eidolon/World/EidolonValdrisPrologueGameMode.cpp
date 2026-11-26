#include "EidolonValdrisPrologueGameMode.h"

#include "../EidolonCampaignSubsystem.h"
#include "../EidolonCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace
{
    using namespace EidolonFirstDescent;
    constexpr double InteractionRange = 220.0;
    constexpr int32 Registration = 0;
    constexpr int32 Descent = 1;
    constexpr int32 Return = 4;
    constexpr int32 Dormitory = 5;
    constexpr int32 Reveal = 6;

    const FString AbyssMap = TEXT("/Game/Eidolon/Maps/AbyssFloor001");
    const FName PrologueMap = TEXT("/Game/Eidolon/Maps/ValdrisPrologue");

    const TCHAR* Display(const TCHAR* English, const TCHAR* Italian)
    {
        return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("it")
            ? Italian : English;
    }

    ATargetPoint* FindAnchor(UWorld* World, FName Tag)
    {
        ATargetPoint* Match = nullptr;
        for (TActorIterator<ATargetPoint> It(World); It; ++It)
        {
            if (!It->ActorHasTag(Tag)) continue;
            if (Match)
            {
                UE_LOG(LogTemp, Error, TEXT("Valdris has multiple targets for %s"), *Tag.ToString());
                return nullptr;
            }
            Match = *It;
        }
        if (!Match) UE_LOG(LogTemp, Error, TEXT("Valdris target %s is missing"), *Tag.ToString());
        return Match;
    }
}

UEidolonCampaignSubsystem* AEidolonValdrisPrologueGameMode::GetCampaign() const
{
    const auto* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UEidolonCampaignSubsystem>() : nullptr;
}

void AEidolonValdrisPrologueGameMode::StartPlay()
{
    AGameModeBase::StartPlay();
    UWorld* World = GetWorld();
    if (!World || UGameplayStatics::GetCurrentLevelName(this) != TEXT("ValdrisPrologue") ||
        FPackageName::GetLongPackagePath(World->GetPackage()->GetName()) != TEXT("/Game/Eidolon/Maps"))
    {
        PromptOverride = Display(TEXT("Valdris prologue layout is unavailable."),
                                 TEXT("Il prologo di Valdris non è disponibile."));
        return;
    }

    if (auto* Anchor = FindAnchor(World, TEXT("veyr_farewell")))
    {
        if (FarewellInteraction.BindTarget(Registration, Steps[Registration].Target, *Anchor))
            Farewell = Anchor;
    }

    const TPair<int32, FName> Layout[] = {
        {Registration, TEXT("guild_registration")}, {Descent, TEXT("abyss_entrance")},
        {Dormitory, TEXT("guild_dormitory")}, {Reveal, TEXT("veyr_reveal_door")}
    };
    for (const auto& Entry : Layout)
    {
        if (auto* Anchor = FindAnchor(World, Entry.Value))
        {
            if (QuestInteraction.BindTarget(Entry.Key, Steps[Entry.Key].Target, *Anchor))
                Anchors.Add(Entry.Key, Anchor);
        }
    }

    if (auto* Campaign = GetCampaign())
        Campaign->CompleteFirstDescentSurfaceReturn();
}

void AEidolonValdrisPrologueGameMode::Interact(AEidolonCharacter* Player)
{
    auto* Campaign = GetCampaign();
    if (!Campaign || !Player || Player != UGameplayStatics::GetPlayerCharacter(this, 0) || Player->IsDefeated()) return;
    PromptOverride.Reset();
    const auto Chapter = Campaign->CurrentChapter();
    const auto SaveFailure = [this]()
    {
        PromptOverride = Display(TEXT("Checkpoint failed. Interact to retry."),
                                 TEXT("Salvataggio fallito. Interagisci per riprovare."));
    };

    if (Chapter.Id == 1)
    {
        const AActor* Target = Farewell.Get();
        if (!Target || !FarewellInteraction.CanReachTarget(*Player, *Target, Registration, InteractionRange)) return;
        if (Chapter.Dialogue.IsValidIndex(FarewellLine))
        {
            Dialogue = Chapter.Dialogue[FarewellLine++];
            return;
        }
        if (!Campaign->BeginValdrisFirstDescent()) { SaveFailure(); return; }
        Dialogue.Reset();
        return;
    }
    if (Chapter.Id != 2 && Chapter.Id != 3) return;

    const int32 Cursor = Campaign->GetFirstDescentCursor();
    const bool bSecondDay = Chapter.Id == 3;
    if (bSecondDay && Campaign->HasFlag(TEXT("chapter_3_riven_spoken")))
    {
        const auto* Guild = Anchors.Find(Registration);
        const auto* Target = Guild ? Guild->Get() : nullptr;
        if (!Target || !QuestInteraction.CanReachTarget(*Player, *Target, Registration, InteractionRange)) return;
        if (!bWarningHeard)
        {
            Dialogue = Display(TEXT("Adventurer: Whatever happens, don't tell the Guild Master."),
                               TEXT("Avventuriero: Qualunque cosa accada, non parlarne con il Maestro della Gilda."));
            bWarningHeard = true;
            return;
        }
        if (!Campaign->FinishSecondDay()) SaveFailure();
        return;
    }
    if (bSecondDay || (Cursor >= Descent && Cursor <= Return))
    {
        const auto* Entrance = Anchors.Find(Descent);
        const auto* Target = Entrance ? Entrance->Get() : nullptr;
        if (!Target || !QuestInteraction.CanReachTarget(*Player, *Target, Descent, InteractionRange)) return;

        if (Cursor == Return && Campaign->HasPendingFirstDescentSurfaceReturn())
        {
            if (!Campaign->CompleteFirstDescentSurfaceReturn()) SaveFailure();
            return;
        }
        if (!FPackageName::DoesPackageExist(AbyssMap))
        {
            PromptOverride = Display(TEXT("The route to the Abyss is unavailable."),
                                     TEXT("Il passaggio per l'Abisso non è disponibile."));
            return;
        }
        UGameplayStatics::OpenLevel(this, FName(*AbyssMap));
        return;
    }

    const auto* Entry = Anchors.Find(Cursor);
    const auto* Target = Entry ? Entry->Get() : nullptr;
    if (!Target || Cursor < 0 || static_cast<std::size_t>(Cursor) >= Steps.size()) return;

    const Event Input{2, "mq_02", static_cast<std::size_t>(Cursor), Steps[Cursor]};
    if (!QuestInteraction.CanInteract(*Player, *Target, Cursor, Input, InteractionRange)) return;
    if (!Campaign->CommitFirstDescentEvent(Input)) { SaveFailure(); return; }

    switch (Cursor)
    {
        case Registration:
            Dialogue = Display(TEXT("Registrar: Go down to the first floor and report back alive."),
                               TEXT("Addetto: Scendi al primo piano e torna a rapporto."));
            break;
        case Dormitory:
            Dialogue = Display(TEXT("The next morning, Kael set out for home."),
                               TEXT("La mattina seguente, Kael si incamminò verso casa."));
            break;
        case Reveal:
            Dialogue = Display(TEXT("Noah: Big brother! I've been waiting for you!"),
                               TEXT("Noah: Fratellone! Ti ho aspettato!"));
            break;
    }
}

void AEidolonValdrisPrologueGameMode::ReloadCheckpoint()
{
    if (auto* Campaign = GetCampaign())
        Campaign->LoadCampaign();
    UGameplayStatics::OpenLevel(this, PrologueMap);
}

FString AEidolonValdrisPrologueGameMode::GetPrompt() const
{
    if (!PromptOverride.IsEmpty()) return PromptOverride;
    const auto* Campaign = GetCampaign();
    if (!Campaign) return {};

    const int32 Chapter = Campaign->CurrentChapter().Id;
    if (Chapter == 1)
        return Display(TEXT("Speak to your family."), TEXT("Parla con la tua famiglia."));
    if (Chapter == 3)
        return Campaign->HasFlag(TEXT("chapter_3_riven_spoken"))
            ? Display(TEXT("Speak to the adventurer outside the Guild."), TEXT("Parla con l'avventuriero fuori dalla Gilda."))
            : Display(TEXT("Meet Riven on the first floor of the Abyss."), TEXT("Raggiungi Riven al primo piano dell'Abisso."));
    if (Chapter == 4)
        return Display(TEXT("Next chapter: the hundredth floor."), TEXT("Prossimo capitolo: il centesimo livello."));
    if (Chapter != 2) return {};

    switch (Campaign->GetFirstDescentCursor())
    {
        case Registration:
            return Display(TEXT("Register at the Guild."), TEXT("Registrati alla Gilda."));
        case Descent: case 2: case 3: case Return:
            return Display(TEXT("Use the Abyss entrance."), TEXT("Usa l'ingresso dell'Abisso."));
        case Dormitory:
            return Display(TEXT("Rest at the Guild dormitory."), TEXT("Riposa nel dormitorio della Gilda."));
        case Reveal:
            return Display(TEXT("Return to the Veyr home."), TEXT("Torna a casa Veyr."));
        default:
            return {};
    }
}

FString AEidolonValdrisPrologueGameMode::GetDialogue() const
{
    return Dialogue;
}
