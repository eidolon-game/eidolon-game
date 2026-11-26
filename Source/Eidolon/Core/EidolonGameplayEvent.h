#pragma once
#include "CoreMinimal.h"
#include "EidolonGameplayEvent.generated.h"

// Proposed studio v1 transport. Persistence/deduplication belongs to Save.
USTRUCT(BlueprintType)
struct FEidolonGameplayEvent
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid EventId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString SourceEntityId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString TargetEntityId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString EventType;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ChapterId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString QuestId;

    static bool IsStableId(const FString& Id)
    {
        if (Id.IsEmpty() || Id.Len() > 128 || Id[0] == TCHAR('_') || Id[Id.Len()-1] == TCHAR('_')) return false;
        for (int32 I = 0; I < Id.Len(); ++I)
        {
            const TCHAR C = Id[I];
            if (!((C >= TCHAR('a') && C <= TCHAR('z')) ||
                (C >= TCHAR('0') && C <= TCHAR('9')) || C == TCHAR('_'))) return false;
            if (I > 0 && C == TCHAR('_') && Id[I-1] == TCHAR('_')) return false;
        }
        return true;
    }

    bool IsValid() const
    {
        return EventId.IsValid() && IsStableId(SourceEntityId) &&
            (TargetEntityId.IsEmpty() || IsStableId(TargetEntityId)) &&
            IsStableId(EventType) && ChapterId >= 1 && ChapterId <= 26 &&
            (QuestId.IsEmpty() || IsStableId(QuestId));
    }
};
