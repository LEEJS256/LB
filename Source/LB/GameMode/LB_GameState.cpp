// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LB_GameState.h"

#include "LB.h"
#include "Dungeon/LB_DungeonRoomLoader.h"
#include "Controller/LB_PlayerController.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

void ALB_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALB_GameState, Route);
	DOREPLIFETIME(ALB_GameState, TransitionSnapshot);
}

void ALB_GameState::BeginPlay()
{
	Super::BeginPlay();

	// 늦은 접속: BeginPlay 이전에 도착한 복제값도 같은 경로로 적용한다.
	ApplyDungeonStateLocal();
	if (HasDungeonRoute())
	{
		OnDungeonTransitionChanged.Broadcast(TransitionSnapshot);
	}
}

void ALB_GameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseLoader(PendingRoomLoader);
	PendingRoomSerial = INDEX_NONE;
	bPendingReadyNotified = false;

	ReleaseLoader(CurrentRoomLoader);
	CurrentRoomSerial = INDEX_NONE;

	NotifiedCurrentInstanceKey = NAME_None;
	NotifiedCurrentRoomId = NAME_None;
	NotifiedCurrentSerial = INDEX_NONE;

	Super::EndPlay(EndPlayReason);
}

void ALB_GameState::OnRep_Route()
{
	ApplyDungeonStateLocal();
}

void ALB_GameState::OnRep_TransitionSnapshot()
{
	ApplyDungeonStateLocal();
	OnDungeonTransitionChanged.Broadcast(TransitionSnapshot);
}

bool ALB_GameState::PublishDungeonRoute(const FLB_DungeonRouteState& NewRoute)
{
	if (!HasAuthority())
	{
		return false;
	}
	if (HasDungeonRoute())
	{
		UE_LOG(LogLB, Warning, TEXT("PublishDungeonRoute: Route already published (%s)."), *Route.InstanceKey.ToString());
		return false;
	}

	FString Error;
	if (!NewRoute.Validate(Error))
	{
		UE_LOG(LogLB, Error, TEXT("PublishDungeonRoute: invalid route - %s"), *Error);
		return false;
	}

	Route = NewRoute;
	ApplyDungeonStateLocal();
	ForceNetUpdate();
	return true;
}

bool ALB_GameState::UpdateDungeonTransition(const FLB_DungeonTransitionSnapshot& NewSnapshot)
{
	if (!HasAuthority() || !HasDungeonRoute())
	{
		return false;
	}

	// Room/Entry는 쌍으로 검증한다: Room이 None이면 Entry도 None, 아니면 Entry가 그 방 EntryIds에 선언돼 있어야 한다.
	if (NewSnapshot.CurrentRoomId.IsNone())
	{
		if (!NewSnapshot.CurrentEntryId.IsNone())
		{
			UE_LOG(LogLB, Error, TEXT("UpdateDungeonTransition: CurrentEntryId %s without CurrentRoomId."), *NewSnapshot.CurrentEntryId.ToString());
			return false;
		}
	}
	else
	{
		const FLB_DungeonRoomDefinition* CurrentRoom = FindRoomDefinition(NewSnapshot.CurrentRoomId);
		if (!CurrentRoom || !FLB_DungeonRouteState::RoomHasEntry(*CurrentRoom, NewSnapshot.CurrentEntryId))
		{
			UE_LOG(LogLB, Error, TEXT("UpdateDungeonTransition: invalid current room/entry %s/%s."),
				*NewSnapshot.CurrentRoomId.ToString(), *NewSnapshot.CurrentEntryId.ToString());
			return false;
		}
	}
	if (NewSnapshot.PendingRoomId.IsNone())
	{
		if (!NewSnapshot.PendingEntryId.IsNone())
		{
			UE_LOG(LogLB, Error, TEXT("UpdateDungeonTransition: PendingEntryId %s without PendingRoomId."), *NewSnapshot.PendingEntryId.ToString());
			return false;
		}
	}
	else
	{
		const FLB_DungeonRoomDefinition* PendingRoom = FindRoomDefinition(NewSnapshot.PendingRoomId);
		if (!PendingRoom || NewSnapshot.PendingRoomId == NewSnapshot.CurrentRoomId
			|| !FLB_DungeonRouteState::RoomHasEntry(*PendingRoom, NewSnapshot.PendingEntryId))
		{
			UE_LOG(LogLB, Error, TEXT("UpdateDungeonTransition: invalid pending room/entry %s/%s."),
				*NewSnapshot.PendingRoomId.ToString(), *NewSnapshot.PendingEntryId.ToString());
			return false;
		}
	}
	if (NewSnapshot.TransitionSerial < 0)
	{
		return false;
	}

	TransitionSnapshot = NewSnapshot;
	ApplyDungeonStateLocal();
	OnDungeonTransitionChanged.Broadcast(TransitionSnapshot);
	ForceNetUpdate();
	return true;
}

bool ALB_GameState::CompleteDungeonTransition()
{
	if (!HasAuthority() || TransitionSnapshot.PendingRoomId.IsNone())
	{
		return false;
	}

	FLB_DungeonTransitionSnapshot NewSnapshot = TransitionSnapshot;
	NewSnapshot.CurrentRoomId = TransitionSnapshot.PendingRoomId;
	NewSnapshot.CurrentEntryId = TransitionSnapshot.PendingEntryId;
	NewSnapshot.PendingRoomId = NAME_None;
	NewSnapshot.PendingEntryId = NAME_None;
	NewSnapshot.ActiveExitId = NAME_None;
	// TransitionSerial은 유지: 승격된 방의 세대가 된다.
	return UpdateDungeonTransition(NewSnapshot);
}

void ALB_GameState::ReceiveClientRoomReady(ALB_PlayerController* Reporter, FName InstanceKey, FName RoomId, int32 TransitionSerial)
{
	if (!HasAuthority() || !IsValid(Reporter) || !HasDungeonRoute())
	{
		return;
	}

	if (InstanceKey != Route.InstanceKey
		|| TransitionSnapshot.PendingRoomId.IsNone()
		|| RoomId != TransitionSnapshot.PendingRoomId
		|| TransitionSerial != TransitionSnapshot.TransitionSerial)
	{
		UE_LOG(LogLB, Verbose, TEXT("ReceiveClientRoomReady: stale report ignored (%s/%s/%d)."),
			*InstanceKey.ToString(), *RoomId.ToString(), TransitionSerial);
		return;
	}

	OnClientPendingRoomReady.Broadcast(Reporter, RoomId, TransitionSerial);
}

void ALB_GameState::ReceiveClientCurrentRoomReady(ALB_PlayerController* Reporter, FName InstanceKey, FName RoomId, int32 TransitionSerial)
{
	if (!HasAuthority() || !IsValid(Reporter) || !HasDungeonRoute())
	{
		return;
	}

	if (InstanceKey != Route.InstanceKey
		|| !TransitionSnapshot.PendingRoomId.IsNone()
		|| TransitionSnapshot.CurrentRoomId.IsNone()
		|| RoomId != TransitionSnapshot.CurrentRoomId
		|| TransitionSerial != TransitionSnapshot.TransitionSerial
		|| !IsStableDungeonPhase(TransitionSnapshot.Phase))
	{
		UE_LOG(LogLB, Verbose, TEXT("ReceiveClientCurrentRoomReady: stale report ignored (%s/%s/%d)."),
			*InstanceKey.ToString(), *RoomId.ToString(), TransitionSerial);
		return;
	}

	OnClientCurrentRoomReady.Broadcast(Reporter, RoomId, TransitionSerial);
}

bool ALB_GameState::IsStableDungeonPhase(ELB_DungeonTransitionPhase Phase)
{
	return Phase == ELB_DungeonTransitionPhase::Waiting || Phase == ELB_DungeonTransitionPhase::CountingDown;
}

bool ALB_GameState::IsLocalStableCurrentRoomReady() const
{
	const FLB_DungeonTransitionSnapshot& Snapshot = TransitionSnapshot;
	return HasDungeonRoute()
		&& Snapshot.PendingRoomId.IsNone()
		&& !Snapshot.CurrentRoomId.IsNone()
		&& IsStableDungeonPhase(Snapshot.Phase)
		&& IsValid(CurrentRoomLoader)
		&& CurrentRoomLoader->IsRoomReady()
		&& CurrentRoomLoader->GetRequestedRoomId() == Snapshot.CurrentRoomId
		&& CurrentRoomSerial == Snapshot.TransitionSerial;
}

void ALB_GameState::TryNotifyLocalCurrentRoomReady()
{
	// 서버 World는 보고 주체가 아니다(Listen 호스트는 파티 Teleport로 이동).
	if (HasAuthority() || !IsLocalStableCurrentRoomReady())
	{
		return;
	}

	const FName InstanceKey = Route.InstanceKey;
	const FName RoomId = TransitionSnapshot.CurrentRoomId;
	const int32 Serial = TransitionSnapshot.TransitionSerial;
	if (NotifiedCurrentInstanceKey == InstanceKey && NotifiedCurrentRoomId == RoomId && NotifiedCurrentSerial == Serial)
	{
		return;
	}

	NotifiedCurrentInstanceKey = InstanceKey;
	NotifiedCurrentRoomId = RoomId;
	NotifiedCurrentSerial = Serial;
	OnLocalCurrentRoomReady.Broadcast(InstanceKey, RoomId, Serial);
}

void ALB_GameState::HandleCurrentRoomReady(ALB_DungeonRoomLoader* Loader)
{
	if (Loader && Loader == CurrentRoomLoader)
	{
		TryNotifyLocalCurrentRoomReady();
	}
}

const FLB_DungeonRoomDefinition* ALB_GameState::FindRoomDefinition(FName RoomId) const
{
	if (RoomId.IsNone())
	{
		return nullptr;
	}
	return Route.Rooms.FindByPredicate([RoomId](const FLB_DungeonRoomDefinition& Room)
	{
		return Room.RoomId == RoomId;
	});
}

ULevel* ALB_GameState::GetPendingRoomLevel() const
{
	return PendingRoomLoader ? PendingRoomLoader->GetLoadedRoomLevel() : nullptr;
}

ULevel* ALB_GameState::GetCurrentRoomLevel() const
{
	return CurrentRoomLoader ? CurrentRoomLoader->GetLoadedRoomLevel() : nullptr;
}

const FTransform& ALB_GameState::GetSlotTransformForSerial(int32 TransitionSerial) const
{
	return (TransitionSerial % 2 != 0) ? RoomSlotATransform : RoomSlotBTransform;
}

void ALB_GameState::ApplyDungeonStateLocal()
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || !HasActorBegunPlay() || !HasDungeonRoute())
	{
		return;
	}

	const FLB_DungeonTransitionSnapshot& Snapshot = TransitionSnapshot;

	// 1) 승격: Pending 로더가 새 Current가 된 경우.
	if (PendingRoomLoader
		&& Snapshot.PendingRoomId.IsNone()
		&& Snapshot.CurrentRoomId == PendingRoomLoader->GetRequestedRoomId()
		&& Snapshot.TransitionSerial == PendingRoomSerial)
	{
		ReleaseLoader(CurrentRoomLoader);

		PendingRoomLoader->OnRoomReady.RemoveAll(this);
		PendingRoomLoader->OnRoomFailed.RemoveAll(this);
		PendingRoomLoader->OnRoomFailed.AddDynamic(this, &ALB_GameState::HandleCurrentRoomFailed);
		// 전환 중 접속해 아직 표시 전인 Client도 승격 후 표시 완료를 받을 수 있게 한다(이미 Ready면 아래 3)에서 처리).
		PendingRoomLoader->OnRoomReady.AddDynamic(this, &ALB_GameState::HandleCurrentRoomReady);

		CurrentRoomLoader = PendingRoomLoader;
		CurrentRoomSerial = PendingRoomSerial;
		PendingRoomLoader = nullptr;
		PendingRoomSerial = INDEX_NONE;
		bPendingReadyNotified = false;
	}

	// 2) 더 이상 유효하지 않은 Pending 로더 해제(취소·세대 변경).
	if (PendingRoomLoader
		&& (Snapshot.PendingRoomId != PendingRoomLoader->GetRequestedRoomId() || Snapshot.TransitionSerial != PendingRoomSerial))
	{
		ReleaseLoader(PendingRoomLoader);
		PendingRoomSerial = INDEX_NONE;
		bPendingReadyNotified = false;
	}

	// 3) 안정 상태: Current 방 복구(늦은 접속).
	if (Snapshot.PendingRoomId.IsNone())
	{
		if (CurrentRoomLoader
			&& (Snapshot.CurrentRoomId != CurrentRoomLoader->GetRequestedRoomId() || Snapshot.TransitionSerial != CurrentRoomSerial))
		{
			ReleaseLoader(CurrentRoomLoader);
			CurrentRoomSerial = INDEX_NONE;
		}

		if (!CurrentRoomLoader && !Snapshot.CurrentRoomId.IsNone())
		{
			if (const FLB_DungeonRoomDefinition* Room = FindRoomDefinition(Snapshot.CurrentRoomId))
			{
				const FLB_DungeonRoomDefinition RoomCopy = *Room;
				CurrentRoomLoader = SpawnRoomLoader();
				if (CurrentRoomLoader)
				{
					CurrentRoomSerial = Snapshot.TransitionSerial;
					CurrentRoomLoader->OnRoomFailed.AddDynamic(this, &ALB_GameState::HandleCurrentRoomFailed);
					CurrentRoomLoader->OnRoomReady.AddDynamic(this, &ALB_GameState::HandleCurrentRoomReady);
					CurrentRoomLoader->LoadRoomLocal(Route.InstanceKey, CurrentRoomSerial, RoomCopy, GetSlotTransformForSerial(CurrentRoomSerial));
				}
			}
			else
			{
				UE_LOG(LogLB, Error, TEXT("ApplyDungeonStateLocal: CurrentRoomId %s not in route."), *Snapshot.CurrentRoomId.ToString());
			}
		}

		// 이미 표시된 Current(승격된 Pending 포함)가 안정 Phase에 도달하면 한 번 알린다.
		TryNotifyLocalCurrentRoomReady();
		return;
	}

	// 4) Pending 방 로드(세대당 한 번).
	if (PendingRoomLoader)
	{
		return;
	}

	if (CurrentRoomLoader && (CurrentRoomSerial % 2) == (Snapshot.TransitionSerial % 2))
	{
		UE_LOG(LogLB, Error, TEXT("ApplyDungeonStateLocal: pending serial %d maps to current slot (current serial %d)."),
			Snapshot.TransitionSerial, CurrentRoomSerial);
		return;
	}

	const FLB_DungeonRoomDefinition* Room = FindRoomDefinition(Snapshot.PendingRoomId);
	if (!Room)
	{
		UE_LOG(LogLB, Error, TEXT("ApplyDungeonStateLocal: PendingRoomId %s not in route."), *Snapshot.PendingRoomId.ToString());
		return;
	}

	const FLB_DungeonRoomDefinition RoomCopy = *Room;
	PendingRoomLoader = SpawnRoomLoader();
	if (!PendingRoomLoader)
	{
		return;
	}

	// 동기 실패/완료 delegate도 받을 수 있도록 멤버 대입·바인딩 후 로드한다.
	PendingRoomSerial = Snapshot.TransitionSerial;
	bPendingReadyNotified = false;
	PendingRoomLoader->OnRoomReady.AddDynamic(this, &ALB_GameState::HandlePendingRoomReady);
	PendingRoomLoader->OnRoomFailed.AddDynamic(this, &ALB_GameState::HandlePendingRoomFailed);
	PendingRoomLoader->LoadRoomLocal(Route.InstanceKey, PendingRoomSerial, RoomCopy, GetSlotTransformForSerial(PendingRoomSerial));
}

ALB_DungeonRoomLoader* ALB_GameState::SpawnRoomLoader()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ALB_DungeonRoomLoader* Loader = World->SpawnActor<ALB_DungeonRoomLoader>(ALB_DungeonRoomLoader::StaticClass(), FTransform::Identity, Params);
	if (Loader)
	{
		// 로컬 전용 로더: 각 World가 독립적으로 생성한다.
		Loader->SetReplicates(false);
	}
	else
	{
		UE_LOG(LogLB, Error, TEXT("SpawnRoomLoader: failed to spawn room loader."));
	}
	return Loader;
}

void ALB_GameState::ReleaseLoader(TObjectPtr<ALB_DungeonRoomLoader>& Loader)
{
	if (IsValid(Loader))
	{
		Loader->OnRoomReady.RemoveAll(this);
		Loader->OnRoomFailed.RemoveAll(this);
		Loader->UnloadRoomLocal();
		Loader->Destroy();
	}
	Loader = nullptr;
}

void ALB_GameState::HandlePendingRoomReady(ALB_DungeonRoomLoader* Loader)
{
	if (!Loader || Loader != PendingRoomLoader || bPendingReadyNotified)
	{
		return;
	}
	bPendingReadyNotified = true;

	const FName RoomId = Loader->GetRequestedRoomId();
	if (HasAuthority())
	{
		OnServerPendingRoomReady.Broadcast(RoomId, PendingRoomSerial);
	}
	else
	{
		OnLocalPendingRoomReady.Broadcast(Route.InstanceKey, RoomId, PendingRoomSerial);
	}
}

void ALB_GameState::HandlePendingRoomFailed(ALB_DungeonRoomLoader* Loader)
{
	if (!Loader || Loader != PendingRoomLoader)
	{
		return;
	}

	// 같은 세대는 재시도하지 않는다(실패 로더를 유지해 중복 생성 방지).
	UE_LOG(LogLB, Error, TEXT("Pending room %s (serial %d) failed to load."), *Loader->GetRequestedRoomId().ToString(), PendingRoomSerial);
	OnLocalPendingRoomFailed.Broadcast(Loader->GetRequestedRoomId(), PendingRoomSerial);
}

void ALB_GameState::HandleCurrentRoomFailed(ALB_DungeonRoomLoader* Loader)
{
	if (Loader && Loader == CurrentRoomLoader)
	{
		UE_LOG(LogLB, Error, TEXT("Current room %s (serial %d) failed to load."), *Loader->GetRequestedRoomId().ToString(), CurrentRoomSerial);
	}
}
