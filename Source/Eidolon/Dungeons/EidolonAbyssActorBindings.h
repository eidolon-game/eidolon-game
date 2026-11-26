#pragma once

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

namespace EidolonAbyss
{
    template <typename ActorType>
    ActorType* FindTaggedActor(UWorld& World, FName Tag)
    {
        ActorType* Found = nullptr;
        for (TActorIterator<AActor> It(&World); It; ++It)
        {
            if (It->IsActorBeingDestroyed() || !It->ActorHasTag(Tag)) continue;
            if (Found || !It->HasAuthority()) return nullptr;
            Found = Cast<ActorType>(*It);
            if (!Found) return nullptr;
        }
        return Found;
    }
}
