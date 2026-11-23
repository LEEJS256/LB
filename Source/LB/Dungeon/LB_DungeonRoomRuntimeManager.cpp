// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon/LB_DungeonRoomRuntimeManager.h"

#include "LB.h"
#include "Dungeon/LB_DungeonGameplayMarkers.h"
#include "Character/Monster/LB_MonsterBase.h"
#include "Object/Destructible/LB_DestructibleProp.h"
#include "Engine/Level.h"
#include "Engine/World.h"

ALB_DungeonRoomRuntimeManager::ALB_DungeonRoomRuntimeManager()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;
	SetReplicatingMovement(false);
	bNetLoadOnClient = false;
}

bool ALB_DungeonRoomRuntimeManager::IsServerWorld() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

void ALB_DungeonRoomRuntimeManager::SetSessionInstanceKey(FName InInstanceKey)
{
	if (SessionInstanceKey == InInstanceKey)
	{
		return;
	}

	if (IsRoomActive())
	{
		DeactivateRoom();
	}
	RoomStates.Reset();
	SessionInstanceKey = InInstanceKey;
}

const FLB_DungeonRoomRuntimeState* ALB_DungeonRoomRuntimeManager::GetRoomState() const
{
	return ActiveRoomId.IsNone() ? nullptr : RoomStates.Find(ActiveRoomId);
}

const FLB_DungeonRoomRuntimeState* ALB_DungeonRoomRuntimeManager::FindRoomState(FName RoomId) const
{
	return RoomStates.Find(RoomId);
}

bool ALB_DungeonRoomRuntimeManager::ActivateRoom(FName RoomId, ULevel* LoadedLevel)
{
	UWorld* World = GetWorld();
	if (!IsServerWorld() || !World)
	{
		UE_LOG(LogLB, Warning, TEXT("RoomRuntime.ActivateRoom: server only."));
		return false;
	}

	if (RoomId.IsNone() || !IsValid(LoadedLevel))
	{
		UE_LOG(LogLB, Warning, TEXT("RoomRuntime.ActivateRoom: invalid RoomId or Level (%s)."), *RoomId.ToString());
		return false;
	}

	// 동시 활성화 규칙
	if (IsRoomActive())
	{
		if (ActiveRoomId == RoomId && ActiveLevel.Get() == LoadedLevel)
		{
			return true;
		}
		UE_LOG(LogLB, Warning, TEXT("RoomRuntime.ActivateRoom: room %s already active, reject %s."),
			*ActiveRoomId.ToString(), *RoomId.ToString());
		return false;
	}

	if (LoadedLevel->OwningWorld != World || LoadedLevel == World->PersistentLevel || !LoadedLevel->bIsVisible)
	{
		UE_LOG(LogLB, Warning, TEXT("RoomRuntime.ActivateRoom: level is not a visible room streaming level (%s)."), *RoomId.ToString());
		return false;
	}

	// ---- 1) Marker 수집 및 전체 검증 (Spawn 전) ----
	TArray<ALB_DungeonGameplaySpawnMarker*> Markers;
	for (AActor* Actor : LoadedLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		if (Actor->IsA<ALB_DungeonMonsterSpawnMarker>() || Actor->IsA<ALB_DungeonPropSpawnMarker>())
		{
			Markers.Add(CastChecked<ALB_DungeonGameplaySpawnMarker>(Actor));
		}
	}

	{
		TArray<const ALB_DungeonGameplaySpawnMarker*> ConstMarkers(Markers);
		FString Error;
		if (!ALB_DungeonGameplaySpawnMarker::ValidateUniqueSpawnIds(ConstMarkers, Error))
		{
			UE_LOG(LogLB, Warning, TEXT("RoomRuntime.ActivateRoom: SpawnId validation failed (%s): %s"), *RoomId.ToString(), *Error);
			return false;
		}
	}

	for (const ALB_DungeonGameplaySpawnMarker* Marker : Markers)
	{
		UClass* SpawnClass = nullptr;
		if (const ALB_DungeonMonsterSpawnMarker* MonsterMarker = Cast<ALB_DungeonMonsterSpawnMarker>(Marker))
		{
			SpawnClass = MonsterMarker->GetMonsterClass().Get();
		}
		else if (const ALB_DungeonPropSpawnMarker* PropMarker = Cast<ALB_DungeonPropSpawnMarker>(Marker))
		{
			SpawnClass = PropMarker->GetPropClass().Get();
		}

		if (!Marker->HasSpawnClass() || !SpawnClass || SpawnClass->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(LogLB, Warning, TEXT("RoomRuntime.ActivateRoom: marker %s has no valid spawn class (%s)."),
				*Marker->GetSpawnId().ToString(), *RoomId.ToString());
			return false;
		}
		if (Marker->GetSpawnLevel() != LoadedLevel)
		{
			UE_LOG(LogLB, Warning, TEXT("RoomRuntime.ActivateRoom: marker %s level mismatch (%s)."),
				*Marker->GetSpawnId().ToString(), *RoomId.ToString());
			return false;
		}
	}

	// ---- 2) 원자적 Spawn ----
	ActiveRoomId = RoomId;
	ActiveLevel = LoadedLevel;
	ManagedSpawns.Reset();
	LivingMonsterIds.Reset();
	bActivationInProgress = true;

	const FLB_DungeonRoomRuntimeState* ExistingState = RoomStates.Find(RoomId);

	FActorSpawnParameters Params;
	Params.OverrideLevel = LoadedLevel;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	bool bFailed = false;
	for (const ALB_DungeonGameplaySpawnMarker* Marker : Markers)
	{
		const FName SpawnId = Marker->GetSpawnId();
		const FTransform SpawnTransform = Marker->GetSpawnTransform();

		if (const ALB_DungeonMonsterSpawnMarker* MonsterMarker = Cast<ALB_DungeonMonsterSpawnMarker>(Marker))
		{
			if (ExistingState && ExistingState->IsMonsterDefeated(SpawnId))
			{
				continue;
			}

			ALB_MonsterBase* Monster = World->SpawnActor<ALB_MonsterBase>(MonsterMarker->GetMonsterClass(), SpawnTransform, Params);
			if (!IsValid(Monster) || Monster->GetLevel() != LoadedLevel)
			{
				if (IsValid(Monster))
				{
					Monster->Destroy();
				}
				bFailed = true;
				break;
			}

			FManagedSpawn& Managed = ManagedSpawns.AddDefaulted_GetRef();
			Managed.SpawnId = SpawnId;
			Managed.Actor = Monster;
			Managed.bMonster = true;
			Managed.Handle = Monster->GetOnMonsterDefeated().AddUObject(this, &ALB_DungeonRoomRuntimeManager::HandleMonsterDefeated, SpawnId);
			if (!Managed.Handle.IsValid())
			{
				bFailed = true;
				break;
			}
			LivingMonsterIds.Add(SpawnId);
		}
		else if (const ALB_DungeonPropSpawnMarker* PropMarker = Cast<ALB_DungeonPropSpawnMarker>(Marker))
		{
			if (ExistingState && ExistingState->IsPropBroken(SpawnId))
			{
				continue;
			}

			ALB_DestructibleProp* Prop = World->SpawnActor<ALB_DestructibleProp>(PropMarker->GetPropClass(), SpawnTransform, Params);
			if (!IsValid(Prop) || Prop->GetLevel() != LoadedLevel)
			{
				if (IsValid(Prop))
				{
					Prop->Destroy();
				}
				bFailed = true;
				break;
			}

			FManagedSpawn& Managed = ManagedSpawns.AddDefaulted_GetRef();
			Managed.SpawnId = SpawnId;
			Managed.Actor = Prop;
			Managed.bMonster = false;
			Managed.Handle = Prop->OnPropBroken().AddUObject(this, &ALB_DungeonRoomRuntimeManager::HandlePropBroken, SpawnId);
			if (!Managed.Handle.IsValid())
			{
				bFailed = true;
				break;
			}
		}
	}

	// Spawn 중 BeginPlay 등으로 이미 제거된 Actor가 있으면 실패로 본다.
	if (!bFailed)
	{
		for (const FManagedSpawn& Managed : ManagedSpawns)
		{
			if (!Managed.Actor.IsValid())
			{
				bFailed = true;
				break;
			}
		}
	}

	if (bFailed)
	{
		UE_LOG(LogLB, Warning, TEXT("RoomRuntime.ActivateRoom: spawn failed, rolling back (%s)."), *RoomId.ToString());
		// 누적 상태(RoomStates)는 건드리지 않는다.
		ReleaseManagedSpawns(true);
		return false;
	}

	// ---- 3) 커밋 ----
	bActivationInProgress = false;
	FLB_DungeonRoomRuntimeState& State = RoomStates.FindOrAdd(RoomId);
	State.RoomId = RoomId;

	// 바인딩 이전에 이미 결과가 확정된 Actor는 즉시 소비 기록한다(신호 유실 방지).
	for (const FManagedSpawn& Managed : ManagedSpawns)
	{
		AActor* Actor = Managed.Actor.Get();
		if (Managed.bMonster)
		{
			const ALB_MonsterBase* Monster = Cast<ALB_MonsterBase>(Actor);
			if (Monster && Monster->IsDefeatNotified())
			{
				State.RecordMonsterDefeated(Managed.SpawnId);
				LivingMonsterIds.Remove(Managed.SpawnId);
			}
		}
		else
		{
			const ALB_DestructibleProp* Prop = Cast<ALB_DestructibleProp>(Actor);
			if (Prop && Prop->IsBroken())
			{
				State.RecordPropBroken(Managed.SpawnId);
			}
		}
	}

	UE_LOG(LogLB, Log, TEXT("RoomRuntime.ActivateRoom: %s active (markers=%d, spawned=%d, living=%d)."),
		*RoomId.ToString(), Markers.Num(), ManagedSpawns.Num(), LivingMonsterIds.Num());

	BroadcastCombatStateChanged();
	return true;
}

void ALB_DungeonRoomRuntimeManager::DeactivateRoom()
{
	if (!IsServerWorld())
	{
		return;
	}
	ReleaseManagedSpawns(true);
}

void ALB_DungeonRoomRuntimeManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	const UWorld* World = GetWorld();
	const bool bCanDestroy = World && !World->bIsTearingDown
		&& (EndPlayReason == EEndPlayReason::Destroyed || EndPlayReason == EEndPlayReason::RemovedFromWorld);
	ReleaseManagedSpawns(bCanDestroy);
	RoomStates.Reset();

	Super::EndPlay(EndPlayReason);
}

void ALB_DungeonRoomRuntimeManager::UnbindManagedSpawn(FManagedSpawn& Spawn)
{
	AActor* Actor = Spawn.Actor.Get();
	if (Actor && Spawn.Handle.IsValid())
	{
		if (Spawn.bMonster)
		{
			if (ALB_MonsterBase* Monster = Cast<ALB_MonsterBase>(Actor))
			{
				Monster->GetOnMonsterDefeated().Remove(Spawn.Handle);
			}
		}
		else if (ALB_DestructibleProp* Prop = Cast<ALB_DestructibleProp>(Actor))
		{
			Prop->OnPropBroken().Remove(Spawn.Handle);
		}
	}
	Spawn.Handle.Reset();
}

void ALB_DungeonRoomRuntimeManager::ReleaseManagedSpawns(bool bDestroyActors)
{
	// 재진입(제거 중 신호) 방지를 위해 먼저 활성 참조를 정리하고 목록을 분리한다.
	TArray<FManagedSpawn> Spawns = MoveTemp(ManagedSpawns);
	ManagedSpawns.Reset();
	LivingMonsterIds.Reset();
	ActiveRoomId = NAME_None;
	ActiveLevel.Reset();
	bActivationInProgress = false;

	for (FManagedSpawn& Spawn : Spawns)
	{
		UnbindManagedSpawn(Spawn);
	}

	if (bDestroyActors)
	{
		for (FManagedSpawn& Spawn : Spawns)
		{
			AActor* Actor = Spawn.Actor.Get();
			if (IsValid(Actor) && !Actor->IsActorBeingDestroyed())
			{
				Actor->Destroy();
			}
		}
	}
}

void ALB_DungeonRoomRuntimeManager::HandleMonsterDefeated(ALB_MonsterBase* Monster, FName SpawnId)
{
	if (!IsServerWorld() || bActivationInProgress || ActiveRoomId.IsNone() || SpawnId.IsNone())
	{
		return;
	}

	// 1) ID 기록 (멱등)
	FLB_DungeonRoomRuntimeState& State = RoomStates.FindOrAdd(ActiveRoomId);
	State.RoomId = ActiveRoomId;
	State.RecordMonsterDefeated(SpawnId);

	// 2) 생존 집합에서 제거 → 3) 변경 시에만 통지
	if (LivingMonsterIds.Remove(SpawnId) > 0)
	{
		BroadcastCombatStateChanged();
	}
}

void ALB_DungeonRoomRuntimeManager::HandlePropBroken(ALB_DestructibleProp* Prop, const FLB_PropBreakState& BreakState, FName SpawnId)
{
	if (!IsServerWorld() || bActivationInProgress || ActiveRoomId.IsNone() || SpawnId.IsNone())
	{
		return;
	}

	FLB_DungeonRoomRuntimeState& State = RoomStates.FindOrAdd(ActiveRoomId);
	State.RoomId = ActiveRoomId;
	State.RecordPropBroken(SpawnId);
}

void ALB_DungeonRoomRuntimeManager::BroadcastCombatStateChanged()
{
	if (ActiveRoomId.IsNone())
	{
		return;
	}
	OnRoomCombatStateChangedDelegate.Broadcast(ActiveRoomId, LivingMonsterIds.Num());
}
