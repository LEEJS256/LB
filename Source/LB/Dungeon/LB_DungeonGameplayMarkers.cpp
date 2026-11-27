// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon/LB_DungeonGameplayMarkers.h"

#include "Data/Dungeon/LB_DungeonRoomTypes.h"
#include "Components/SceneComponent.h"
#include "Engine/Level.h"
#include "Character/Monster/LB_MonsterBase.h"
#include "Object/Destructible/LB_DestructibleProp.h"

// ---------------------------------------------------------------------------
// ALB_DungeonGameplaySpawnMarker
// ---------------------------------------------------------------------------

ALB_DungeonGameplaySpawnMarker::ALB_DungeonGameplaySpawnMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;
	SetReplicatingMovement(false);
	bNetLoadOnClient = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

bool ALB_DungeonGameplaySpawnMarker::HasValidSpawnId() const
{
	return IsValidSpawnId(SpawnId);
}

bool ALB_DungeonGameplaySpawnMarker::IsValidSpawnId(const FName& InSpawnId)
{
	return FLB_DungeonRouteState::IsValidIdentifier(InSpawnId);
}

FTransform ALB_DungeonGameplaySpawnMarker::GetSpawnTransform() const
{
	if (Root)
	{
		return FTransform(Root->GetComponentRotation(), Root->GetComponentLocation());
	}
	return FTransform(GetActorRotation(), GetActorLocation());
}

ULevel* ALB_DungeonGameplaySpawnMarker::GetSpawnLevel() const
{
	return GetLevel();
}

bool ALB_DungeonGameplaySpawnMarker::ValidateUniqueSpawnIds(
	const TArray<const ALB_DungeonGameplaySpawnMarker*>& Markers, FString& OutError)
{
	OutError.Reset();

	TMap<FName, const ALB_DungeonGameplaySpawnMarker*> Seen;
	Seen.Reserve(Markers.Num());

	for (const ALB_DungeonGameplaySpawnMarker* Marker : Markers)
	{
		if (!Marker)
		{
			continue;
		}

		const FName Id = Marker->GetSpawnId();
		if (!IsValidSpawnId(Id))
		{
			OutError = FString::Printf(TEXT("SpawnMarker '%s': SpawnId '%s' must be non-None and contain only letters, digits, underscore"),
				*Marker->GetName(), *Id.ToString());
			return false;
		}

		if (const ALB_DungeonGameplaySpawnMarker* const* Existing = Seen.Find(Id))
		{
			OutError = FString::Printf(TEXT("SpawnMarker '%s': SpawnId '%s' duplicates SpawnMarker '%s' in the same room"),
				*Marker->GetName(), *Id.ToString(), *(*Existing)->GetName());
			return false;
		}

		Seen.Add(Id, Marker);
	}

	return true;
}

// ---------------------------------------------------------------------------
// ALB_DungeonMonsterSpawnMarker
// ---------------------------------------------------------------------------

ALB_DungeonMonsterSpawnMarker::ALB_DungeonMonsterSpawnMarker()
{
}

bool ALB_DungeonMonsterSpawnMarker::HasSpawnClass() const
{
	return MonsterClass != nullptr;
}

// ---------------------------------------------------------------------------
// ALB_DungeonPropSpawnMarker
// ---------------------------------------------------------------------------

ALB_DungeonPropSpawnMarker::ALB_DungeonPropSpawnMarker()
{
}

bool ALB_DungeonPropSpawnMarker::HasSpawnClass() const
{
	return PropClass != nullptr;
}
