// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LB_GameMode.h"

#include "LB.h"
#include "GameMode/LB_GameState.h"
#include "Controller/LB_PlayerController.h"
#include "Data/Dungeon/LB_DungeonLayoutDataAsset.h"
#include "Dungeon/LB_DungeonRoomMarkers.h"
#include "Dungeon/LB_DungeonRoomRuntimeManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Misc/Guid.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Level.h"

void ALB_GameMode::StartPlay()
{
	Super::StartPlay();

	DungeonGameState = GetGameState<ALB_GameState>();
	if (!DungeonGameState)
	{
		UE_LOG(LogLB, Warning, TEXT("ALB_GameMode::StartPlay: ALB_GameState not found. Dungeon disabled."));
		return;
	}

	BindGameStateDelegates();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		WatchPartyController(It->Get());
	}

	// 던전 시작(Route 게시·첫 방 전환)은 StartDungeon() 명시 호출로만 수행한다.
	// Listen Server PIE에서는 원격 Client의 PostLogin보다 StartPlay가 먼저 실행되기 때문이다.
}

bool ALB_GameMode::StartDungeon()
{
	if (!HasAuthority())
	{
		UE_LOG(LogLB, Warning, TEXT("StartDungeon rejected: no authority."));
		return false;
	}
	if (bDungeonStarted)
	{
		UE_LOG(LogLB, Warning, TEXT("StartDungeon rejected: already started."));
		return false;
	}
	if (!DungeonGameState || !StartupDungeonLayout)
	{
		UE_LOG(LogLB, Warning, TEXT("StartDungeon rejected: GameState %s, StartupDungeonLayout %s."),
			DungeonGameState ? TEXT("valid") : TEXT("null"), StartupDungeonLayout ? TEXT("valid") : TEXT("null"));
		return false;
	}
	if (DungeonGameState->HasDungeonRoute())
	{
		UE_LOG(LogLB, Warning, TEXT("StartDungeon rejected: Route already published."));
		return false;
	}

	// 호출 시점의 접속 파티(유효 PlayerState·Pawn을 가진 비관전자)를 첫 전환 대상으로 잠근다.
	CaptureTransitionParty();
	if (TransitionParty.IsEmpty())
	{
		UE_LOG(LogLB, Warning, TEXT("StartDungeon rejected: no party participants."));
		return false;
	}

	const FName InstanceKey(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
	FLB_DungeonRouteState NewRoute;
	FString Error;
	if (!StartupDungeonLayout->BuildRoute(InstanceKey, NewRoute, Error))
	{
		UE_LOG(LogLB, Error, TEXT("StartDungeon: BuildRoute failed: %s"), *Error);
		TransitionParty.Reset();
		return false;
	}

	if (!DungeonGameState->PublishDungeonRoute(NewRoute))
	{
		UE_LOG(LogLB, Error, TEXT("StartDungeon: PublishDungeonRoute failed."));
		TransitionParty.Reset();
		return false;
	}

	// Route가 게시되면 이후 결과와 무관하게 다시 시작하지 않는다.
	bDungeonStarted = true;

	// 방 Runtime은 Route 게시 성공 뒤 이 InstanceKey 세션 동안 유지된다.
	if (!CreateRoomRuntimeManager(InstanceKey))
	{
		TransitionParty.Reset();
		bTransitionLocked = true;
		FailRoomTransition(TEXT("room runtime manager spawn failed"));
		return false;
	}

	// StartRoom 진입도 Exit 전환과 같은 잠금·Barrier·Fade·Teleport 경로를 사용한다.
	bTransitionLocked = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReevaluateHandle);
	}

	FLB_DungeonTransitionSnapshot Snapshot;
	Snapshot.CurrentRoomId = NAME_None;
	Snapshot.CurrentEntryId = NAME_None;
	Snapshot.PendingRoomId = NewRoute.StartRoomId;
	Snapshot.PendingEntryId = NewRoute.StartEntryId;
	Snapshot.ActiveExitId = NAME_None;
	Snapshot.TransitionSerial = 1;
	Snapshot.Phase = ELB_DungeonTransitionPhase::LoadingNextRoom;
	Snapshot.TotalPlayers = TransitionParty.Num();

	UE_LOG(LogLB, Log, TEXT("StartDungeon: instance %s, start room %s entry %s, party %d."),
		*InstanceKey.ToString(), *NewRoute.StartRoomId.ToString(), *NewRoute.StartEntryId.ToString(), TransitionParty.Num());

	if (!BeginRoomTransition(Snapshot))
	{
		if (DungeonGameState->GetDungeonTransition().Phase != ELB_DungeonTransitionPhase::Failed)
		{
			FailRoomTransition(TEXT("start room publish failed"));
		}
		return false;
	}
	return true;
}

void ALB_GameMode::PostLogin(APlayerController* NewPlayer)
{
	// 전환 중 접속한 Controller는 현재 Barrier·이동 파티에 넣지 않는다.
	// 안정된 Current 방은 GameState 복제로 로드된다.
	Super::PostLogin(NewPlayer);

	WatchPartyController(NewPlayer);
	RequestExitReevaluation();
}

void ALB_GameMode::Logout(AController* Exiting)
{
	if (APlayerController* ExitingPC = Cast<APlayerController>(Exiting))
	{
		UnwatchPartyController(ExitingPC);
		TransitionParty.Remove(ExitingPC);
		CurrentRoomPlaced.Remove(ExitingPC);
		RemovePawnSuppressions(ExitingPC->GetPawn());
	}
	// Controller가 아직 Iterator에 남아 있으므로 다음 Tick에 재평가한다.
	RequestExitReevaluation();

	if (bBarrierActive)
	{
		if (APlayerController* PC = Cast<APlayerController>(Exiting))
		{
			const TWeakObjectPtr<APlayerController> Key(PC);
			if (BarrierParticipants.Remove(Key) > 0)
			{
				ReadyParticipants.Remove(Key);
				TryCompleteRoomBarrier();
			}
		}
	}

	Super::Logout(Exiting);
}

void ALB_GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRoomBarrier();
	ClearTransitionTimers();
	UnbindCurrentRoomExits();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReevaluateHandle);
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			UnwatchPartyController(It->Get());
		}
	}
	TransitionParty.Reset();
	PendingEntry.Reset();
	CurrentEntry.Reset();
	CurrentEntryRoomId = NAME_None;
	CurrentEntryMarkerId = NAME_None;
	CurrentEntrySerial = INDEX_NONE;
	CurrentRoomPlaced.Reset();
	ArrivingPawns.Reset();
	SuppressedExitPawns.Reset();
	// 던전 세션 종료: Runtime Actor·delegate와 누적 Room 상태를 함께 폐기한다.
	DestroyRoomRuntimeManager();
	bTransitionLocked = false;
	bDungeonStarted = false;
	UnbindGameStateDelegates();
	DungeonGameState = nullptr;

	Super::EndPlay(EndPlayReason);
}

bool ALB_GameMode::BeginRoomTransition(const FLB_DungeonTransitionSnapshot& NewSnapshot)
{
	if (!DungeonGameState || !DungeonGameState->HasDungeonRoute() || NewSnapshot.PendingRoomId.IsNone())
	{
		return false;
	}

	ClearRoomBarrier();

	// 참가자 잠금: 유효 PlayerState·Pawn을 가진 비관전자 Controller.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (IsBarrierParticipantCandidate(PC))
		{
			BarrierParticipants.Add(PC);
		}
	}

	bBarrierActive = true;
	BarrierInstanceKey = DungeonGameState->GetDungeonRoute().InstanceKey;
	BarrierRoomId = NewSnapshot.PendingRoomId;
	BarrierSerial = NewSnapshot.TransitionSerial;
	bServerRoomReady = false;
	UE_LOG(LogLB, Log, TEXT("Room barrier opened: %s serial %d, client participants %d."),
		*BarrierRoomId.ToString(), BarrierSerial, BarrierParticipants.Num());

	FLB_DungeonTransitionSnapshot Snapshot = NewSnapshot;
	if (Snapshot.TotalPlayers <= 0)
	{
		Snapshot.TotalPlayers = BarrierParticipants.Num();
	}

	if (RoomReadyTimeoutSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(RoomReadyTimeoutHandle, this, &ALB_GameMode::HandleRoomReadyTimeout,
			RoomReadyTimeoutSeconds, false);
	}

	// 게시 중 서버 로드가 동기 완료될 수 있으므로 Barrier를 먼저 연 뒤 게시하고, 완료 판정은 게시 후로 미룬다.
	bPublishingTransition = true;
	bFailureDuringPublish = false;
	const bool bPublished = DungeonGameState->UpdateDungeonTransition(Snapshot);
	bPublishingTransition = false;

	if (bPublished && bFailureDuringPublish)
	{
		bFailureDuringPublish = false;
		FailRoomTransition(TEXT("local load failed"));
		return false;
	}
	bFailureDuringPublish = false;

	if (!bPublished)
	{
		UE_LOG(LogLB, Error, TEXT("BeginRoomTransition: snapshot publish failed (%s/%d)."),
			*NewSnapshot.PendingRoomId.ToString(), NewSnapshot.TransitionSerial);
		ClearRoomBarrier();
		return false;
	}

	TryCompleteRoomBarrier();
	return true;
}

void ALB_GameMode::HandleRoomBarrierReady()
{
	if (!DungeonGameState)
	{
		return;
	}

	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	UE_LOG(LogLB, Log, TEXT("Room barrier ready: %s (serial %d)."), *Snapshot.PendingRoomId.ToString(), Snapshot.TransitionSerial);

	// Barrier는 잠긴 전환에서만 열린다.
	if (!bTransitionLocked)
	{
		CaptureTransitionParty();
		bTransitionLocked = true;
	}

	const FLB_DungeonRoomDefinition* Room = DungeonGameState->FindRoomDefinition(Snapshot.PendingRoomId);
	ALB_DungeonRoomEntry* Entry = nullptr;
	FString Error;
	if (!Room)
	{
		Error = TEXT("pending room not in route");
	}
	// 목적지 Level에서 PendingEntryId와 정확히 일치하는 Entry를 고른다. 실패하면 Teleport 없이 현재 방에 남고 Failed.
	if (!Room || !ValidateRoomMarkers(DungeonGameState->GetPendingRoomLevel(), *Room, Snapshot.PendingEntryId, Entry, Error))
	{
		UE_LOG(LogLB, Error, TEXT("Room %s entry %s (serial %d) markers invalid: %s"),
			*Snapshot.PendingRoomId.ToString(), *Snapshot.PendingEntryId.ToString(), Snapshot.TransitionSerial, *Error);
		FailRoomTransition(TEXT("room markers invalid"));
		return;
	}
	PendingEntry = Entry;

	FLB_DungeonTransitionSnapshot FadingOut = Snapshot;
	FadingOut.Phase = ELB_DungeonTransitionPhase::FadingOut;
	FadingOut.NearbyPlayers = 0;
	FadingOut.RequiredPlayers = 0;
	FadingOut.CountdownEndServerTime = 0.0;
	if (!DungeonGameState->UpdateDungeonTransition(FadingOut))
	{
		FailRoomTransition(TEXT("fade out publish failed"));
		return;
	}

	BroadcastDungeonFade(true, FadeOutDuration);
	ScheduleTransitionStep(FadeOutDuration, &ALB_GameMode::HandleFadeOutFinished, FadingOut.TransitionSerial);
}

void ALB_GameMode::BindGameStateDelegates()
{
	if (!DungeonGameState)
	{
		return;
	}
	UnbindGameStateDelegates();
	ServerReadyHandle = DungeonGameState->OnServerPendingRoomReady.AddUObject(this, &ALB_GameMode::HandleServerPendingRoomReady);
	ClientReadyHandle = DungeonGameState->OnClientPendingRoomReady.AddUObject(this, &ALB_GameMode::HandleClientPendingRoomReady);
	LocalFailedHandle = DungeonGameState->OnLocalPendingRoomFailed.AddUObject(this, &ALB_GameMode::HandleLocalPendingRoomFailed);
	CurrentReadyHandle = DungeonGameState->OnClientCurrentRoomReady.AddUObject(this, &ALB_GameMode::HandleClientCurrentRoomReady);
}

void ALB_GameMode::UnbindGameStateDelegates()
{
	if (DungeonGameState)
	{
		DungeonGameState->OnServerPendingRoomReady.Remove(ServerReadyHandle);
		DungeonGameState->OnClientPendingRoomReady.Remove(ClientReadyHandle);
		DungeonGameState->OnLocalPendingRoomFailed.Remove(LocalFailedHandle);
		DungeonGameState->OnClientCurrentRoomReady.Remove(CurrentReadyHandle);
	}
	ServerReadyHandle.Reset();
	ClientReadyHandle.Reset();
	LocalFailedHandle.Reset();
	CurrentReadyHandle.Reset();
}

bool ALB_GameMode::IsBarrierParticipantCandidate(const APlayerController* PC)
{
	// Listen Server의 로컬 플레이어는 서버 World 준비로 대표한다.
	// 원격 Client의 서버 측 Controller도 Authority이므로 HasAuthority가 아니라 Local 여부로 구분한다.
	if (!IsValid(PC) || PC->IsLocalController() || !IsValid(PC->GetPawn()))
	{
		return false;
	}
	const APlayerState* PS = PC->PlayerState;
	return IsValid(PS) && !PS->IsSpectator() && !PS->IsOnlyASpectator();
}

bool ALB_GameMode::IsBarrierReport(FName RoomId, int32 TransitionSerial) const
{
	if (!bBarrierActive || !DungeonGameState)
	{
		return false;
	}
	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	return DungeonGameState->GetDungeonRoute().InstanceKey == BarrierInstanceKey
		&& RoomId == BarrierRoomId
		&& TransitionSerial == BarrierSerial
		&& Snapshot.PendingRoomId == BarrierRoomId
		&& Snapshot.TransitionSerial == BarrierSerial;
}

void ALB_GameMode::HandleServerPendingRoomReady(FName RoomId, int32 TransitionSerial)
{
	if (!IsBarrierReport(RoomId, TransitionSerial))
	{
		UE_LOG(LogLB, Verbose, TEXT("Server room ready ignored (%s/%d)."), *RoomId.ToString(), TransitionSerial);
		return;
	}
	bServerRoomReady = true;
	UE_LOG(LogLB, Log, TEXT("Room barrier server ready: %s serial %d, clients %d/%d."),
		*RoomId.ToString(), TransitionSerial, ReadyParticipants.Num(), BarrierParticipants.Num());
	TryCompleteRoomBarrier();
}

void ALB_GameMode::HandleClientPendingRoomReady(ALB_PlayerController* Reporter, FName RoomId, int32 TransitionSerial)
{
	if (!IsBarrierReport(RoomId, TransitionSerial))
	{
		return;
	}

	const TWeakObjectPtr<APlayerController> Key(Reporter);
	if (!BarrierParticipants.Contains(Key))
	{
		UE_LOG(LogLB, Verbose, TEXT("Client room ready from non-participant ignored."));
		return;
	}

	bool bAlreadyReady = false;
	ReadyParticipants.Add(Key, &bAlreadyReady);
	if (bAlreadyReady)
	{
		return;
	}
	UE_LOG(LogLB, Log, TEXT("Room barrier client ready: %s serial %d, clients %d/%d."),
		*RoomId.ToString(), TransitionSerial, ReadyParticipants.Num(), BarrierParticipants.Num());
	TryCompleteRoomBarrier();
}

void ALB_GameMode::HandleLocalPendingRoomFailed(FName RoomId, int32 TransitionSerial)
{
	if (!IsBarrierReport(RoomId, TransitionSerial))
	{
		return;
	}
	if (bPublishingTransition)
	{
		bFailureDuringPublish = true;
		return;
	}
	FailRoomTransition(TEXT("local load failed"));
}

void ALB_GameMode::HandleRoomReadyTimeout()
{
	RoomReadyTimeoutHandle.Invalidate();
	if (!bBarrierActive)
	{
		return;
	}
	UE_LOG(LogLB, Error, TEXT("Room barrier timeout: %s serial %d, server ready %s, clients %d/%d."),
		*BarrierRoomId.ToString(), BarrierSerial, bServerRoomReady ? TEXT("true") : TEXT("false"),
		ReadyParticipants.Num(), BarrierParticipants.Num());
	FailRoomTransition(TEXT("timeout"));
}

// ============================================================================
// 안정 상태 늦은 합류
// ============================================================================

void ALB_GameMode::HandleClientCurrentRoomReady(ALB_PlayerController* Reporter, FName RoomId, int32 TransitionSerial)
{
	// GameState가 InstanceKey/CurrentRoomId/Serial/Pending=None/안정 Phase를 검증했다. 여기서는 서버 진행 상태를 다시 확인한다.
	if (!DungeonGameState || !bDungeonStarted || bTransitionLocked || bBarrierActive)
	{
		return;
	}
	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	if (!Snapshot.PendingRoomId.IsNone()
		|| Snapshot.CurrentRoomId != RoomId
		|| Snapshot.TransitionSerial != TransitionSerial
		|| !ALB_GameState::IsStableDungeonPhase(Snapshot.Phase))
	{
		return;
	}

	// 원격 Client의 비관전자 Controller이고 Pawn이 있어야 한다.
	if (!IsPartyCandidate(Reporter) || Reporter->IsLocalController())
	{
		UE_LOG(LogLB, Verbose, TEXT("Late join report ignored: reporter not eligible."));
		return;
	}

	// 이미 현재 방 Entry로 이동한 파티원/합류 완료자의 보고는 무시한다.
	const TWeakObjectPtr<APlayerController> Key(Reporter);
	if (CurrentRoomPlaced.Contains(Key))
	{
		return;
	}

	ALB_DungeonRoomEntry* Entry = ResolveCurrentRoomEntry();
	if (!Entry)
	{
		UE_LOG(LogLB, Error, TEXT("Late join ignored: current room %s (serial %d) entry unavailable."),
			*RoomId.ToString(), TransitionSerial);
		return;
	}

	// Reporter 한 명에게만 즉시 Fade Out → Teleport → Fade In. 파티 전체 전환은 재실행하지 않는다.
	// 도착 표시를 Teleport 전에 걸어 동기 Overlap callback에서 집계되지 않게 한다(다른 플레이어 집계는 그대로).
	APawn* ArrivingPawn = Reporter->GetPawn();
	MarkArrivingPawn(ArrivingPawn);
	Reporter->ClientSetDungeonFade(true, 0.f);
	const bool bTeleported = TeleportControllerToEntry(Reporter, Entry, 0.f);
	if (bTeleported)
	{
		FinishPawnArrival(ArrivingPawn, Entry);
	}
	else
	{
		ArrivingPawns.Remove(ArrivingPawn);
	}
	Reporter->ClientSetDungeonFade(false, FadeInDuration);
	if (!bTeleported)
	{
		return;
	}
	CurrentRoomPlaced.Add(Key);

	UE_LOG(LogLB, Log, TEXT("Late join placed at entry: %s in %s entry %s (serial %d)."),
		*Reporter->GetName(), *RoomId.ToString(), *Snapshot.CurrentEntryId.ToString(), TransitionSerial);

	// Waiting/CountingDown 중이면 늘어난 Total/Required와 Nearby를 다시 평가한다(진행 중 카운트다운은 재시작하지 않음).
	RequestExitReevaluation();
}

ALB_DungeonRoomEntry* ALB_GameMode::ResolveCurrentRoomEntry()
{
	if (!DungeonGameState)
	{
		return nullptr;
	}
	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();

	ALB_DungeonRoomEntry* Entry = CurrentEntry.Get();
	if (IsValid(Entry)
		&& CurrentEntryRoomId == Snapshot.CurrentRoomId
		&& CurrentEntryMarkerId == Snapshot.CurrentEntryId
		&& CurrentEntrySerial == Snapshot.TransitionSerial
		&& Entry->GetEntryId() == Snapshot.CurrentEntryId)
	{
		return Entry;
	}

	// 보관 Entry가 무효/다른 방·Entry·세대면 서버 Current Level에서 CurrentEntryId Marker를 다시 검증한다.
	CurrentEntry.Reset();
	CurrentEntryRoomId = NAME_None;
	CurrentEntryMarkerId = NAME_None;
	CurrentEntrySerial = INDEX_NONE;

	const FLB_DungeonRoomDefinition* Room = DungeonGameState->FindRoomDefinition(Snapshot.CurrentRoomId);
	FString Error;
	Entry = nullptr;
	if (!Room || !ValidateRoomMarkers(DungeonGameState->GetCurrentRoomLevel(), *Room, Snapshot.CurrentEntryId, Entry, Error))
	{
		UE_LOG(LogLB, Error, TEXT("ResolveCurrentRoomEntry: room %s entry %s (serial %d) invalid: %s"),
			*Snapshot.CurrentRoomId.ToString(), *Snapshot.CurrentEntryId.ToString(), Snapshot.TransitionSerial,
			Room ? *Error : TEXT("room not in route"));
		return nullptr;
	}

	CurrentEntry = Entry;
	CurrentEntryRoomId = Snapshot.CurrentRoomId;
	CurrentEntryMarkerId = Snapshot.CurrentEntryId;
	CurrentEntrySerial = Snapshot.TransitionSerial;
	return Entry;
}

void ALB_GameMode::TryCompleteRoomBarrier()
{
	if (!bBarrierActive || bPublishingTransition || !bServerRoomReady)
	{
		return;
	}

	// 파괴된 참가자는 Logout을 거치지 않았더라도 대기 대상에서 제외한다.
	for (auto It = BarrierParticipants.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			ReadyParticipants.Remove(*It);
			It.RemoveCurrent();
		}
	}

	for (const TWeakObjectPtr<APlayerController>& Participant : BarrierParticipants)
	{
		if (!ReadyParticipants.Contains(Participant))
		{
			return;
		}
	}

	// Barrier를 먼저 닫아 한 번만 실행되게 한다.
	ClearRoomBarrier();
	HandleRoomBarrierReady();
}

void ALB_GameMode::ClearRoomBarrier()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RoomReadyTimeoutHandle);
	}
	bBarrierActive = false;
	BarrierInstanceKey = NAME_None;
	BarrierRoomId = NAME_None;
	BarrierSerial = INDEX_NONE;
	bServerRoomReady = false;
	BarrierParticipants.Reset();
	ReadyParticipants.Reset();
}

void ALB_GameMode::FailRoomTransition(const TCHAR* Reason)
{
	UE_LOG(LogLB, Error, TEXT("Room transition failed (%s): %s serial %d."), Reason, *BarrierRoomId.ToString(), BarrierSerial);

	ClearRoomBarrier();
	ClearTransitionTimers();
	UnbindCurrentRoomExits();
	PendingEntry.Reset();
	// 관리 Actor·구독만 정리하고 누적 Room 상태는 세션 종료까지 유지한다.
	DeactivateRoomRuntime();
	// 재시도 UX는 범위 밖: 잠금을 유지해 Exit 평가·추가 전환을 막는다.
	bTransitionLocked = true;

	// Fade·Teleport·자동 재시도 없이 실패 상태만 남긴다.
	if (DungeonGameState)
	{
		FLB_DungeonTransitionSnapshot Failed = DungeonGameState->GetDungeonTransition();
		// Fade 도중 실패하면 화면만 복구한다(재시도 없음).
		const bool bWasFading = Failed.Phase == ELB_DungeonTransitionPhase::FadingOut
			|| Failed.Phase == ELB_DungeonTransitionPhase::FadingIn;
		if (bWasFading)
		{
			BroadcastDungeonFade(false, FadeInDuration);
		}
		Failed.Phase = ELB_DungeonTransitionPhase::Failed;
		Failed.NearbyPlayers = 0;
		Failed.RequiredPlayers = 0;
		Failed.CountdownEndServerTime = 0.0;
		DungeonGameState->UpdateDungeonTransition(Failed);
	}
}

// ============================================================================
// 파티
// ============================================================================

bool ALB_GameMode::IsPartyCandidate(const APlayerController* PC)
{
	// Barrier 참가자와 달리 Listen Server 로컬 플레이어도 이동 파티에 포함한다.
	if (!IsValid(PC) || !IsValid(PC->GetPawn()))
	{
		return false;
	}
	const APlayerState* PS = PC->PlayerState;
	return IsValid(PS) && !PS->IsSpectator() && !PS->IsOnlyASpectator();
}

void ALB_GameMode::GatherParty(TArray<APlayerController*>& OutParty) const
{
	OutParty.Reset();
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (IsPartyCandidate(PC))
		{
			OutParty.Add(PC);
		}
	}
	// Entry 배치 순서를 결정적으로 유지한다.
	OutParty.Sort([](const APlayerController& A, const APlayerController& B)
	{
		return A.PlayerState->GetPlayerId() < B.PlayerState->GetPlayerId();
	});
}

void ALB_GameMode::CaptureTransitionParty()
{
	TArray<APlayerController*> Party;
	GatherParty(Party);
	TransitionParty.Reset(Party.Num());
	for (APlayerController* PC : Party)
	{
		TransitionParty.Add(PC);
	}
}

void ALB_GameMode::WatchPartyController(APlayerController* PC)
{
	if (!IsValid(PC))
	{
		return;
	}
	PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ALB_GameMode::HandlePossessedPawnChanged);
	WatchPartyPawn(PC->GetPawn());
}

void ALB_GameMode::UnwatchPartyController(APlayerController* PC)
{
	if (!IsValid(PC))
	{
		return;
	}
	PC->OnPossessedPawnChanged.RemoveDynamic(this, &ALB_GameMode::HandlePossessedPawnChanged);
	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->OnDestroyed.RemoveDynamic(this, &ALB_GameMode::HandlePartyPawnDestroyed);
	}
}

void ALB_GameMode::WatchPartyPawn(APawn* Pawn)
{
	if (IsValid(Pawn))
	{
		Pawn->OnDestroyed.AddUniqueDynamic(this, &ALB_GameMode::HandlePartyPawnDestroyed);
	}
}

void ALB_GameMode::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	WatchPartyPawn(NewPawn);
	RequestExitReevaluation();
}

void ALB_GameMode::HandlePartyPawnDestroyed(AActor* DestroyedActor)
{
	// 무효화되는 Pawn의 도착 억제를 모두 정리한다.
	RemovePawnSuppressions(Cast<APawn>(DestroyedActor));
	// Destroy 통지 시점에는 Pawn이 아직 유효할 수 있어 다음 Tick에 평가한다.
	RequestExitReevaluation();
}

void ALB_GameMode::RequestExitReevaluation()
{
	UWorld* World = GetWorld();
	if (bTransitionLocked || !World || World->bIsTearingDown || ReevaluateHandle.IsValid())
	{
		return;
	}
	ReevaluateHandle = World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &ALB_GameMode::HandleDeferredExitReevaluation));
}

void ALB_GameMode::HandleDeferredExitReevaluation()
{
	ReevaluateHandle.Invalidate();
	EvaluateAllExits();
}

// ============================================================================
// Exit 평가
// ============================================================================

void ALB_GameMode::BindCurrentRoomExits()
{
	UnbindCurrentRoomExits();
	if (!DungeonGameState)
	{
		return;
	}

	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	const FLB_DungeonRoomDefinition* Room = DungeonGameState->FindRoomDefinition(Snapshot.CurrentRoomId);
	if (!Room || !Snapshot.PendingRoomId.IsNone())
	{
		return;
	}

	BoundExitsRoomId = Snapshot.CurrentRoomId;
	BoundExitsSerial = Snapshot.TransitionSerial;

	// 종료 방: delegate를 바인딩하지 않는다.
	if (Room->Exits.IsEmpty())
	{
		UE_LOG(LogLB, Log, TEXT("Room %s is a terminal room (no exits)."), *Room->RoomId.ToString());
		return;
	}

	ULevel* Level = DungeonGameState->GetCurrentRoomLevel();
	if (!Level)
	{
		UE_LOG(LogLB, Error, TEXT("BindCurrentRoomExits: current room %s level not visible."), *Room->RoomId.ToString());
		return;
	}

	for (AActor* Actor : Level->Actors)
	{
		ALB_DungeonExit* Exit = Cast<ALB_DungeonExit>(Actor);
		if (!IsValid(Exit) || FindTargetRoomForExit(Exit->GetExitId()).IsNone())
		{
			continue;
		}
		Exit->OnExitPresenceChanged.AddUObject(this, &ALB_GameMode::HandleExitPresenceChanged);
		BoundExits.Add(Exit);
		// 바인딩 전(Fade In 중)에 이미 Exit를 벗어난 도착 Pawn의 억제를 해제한다.
		ReleaseExitSuppressions(Exit);
	}
	UE_LOG(LogLB, Log, TEXT("Room %s (serial %d): bound %d exits."), *BoundExitsRoomId.ToString(), BoundExitsSerial, BoundExits.Num());
}

void ALB_GameMode::UnbindCurrentRoomExits()
{
	for (const TWeakObjectPtr<ALB_DungeonExit>& WeakExit : BoundExits)
	{
		if (ALB_DungeonExit* Exit = WeakExit.Get())
		{
			Exit->OnExitPresenceChanged.RemoveAll(this);
		}
		// 해제되는 Exit의 도착 억제만 정리한다(아직 바인딩 전인 도착 방 Exit 억제는 유지).
		SuppressedExitPawns.Remove(WeakExit);
	}
	BoundExits.Reset();
	PruneExitSuppressions();
	BoundExitsRoomId = NAME_None;
	BoundExitsSerial = INDEX_NONE;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExitCountdownHandle);
	}
}

void ALB_GameMode::HandleExitPresenceChanged(ALB_DungeonExit* Exit)
{
	const bool bBound = BoundExits.ContainsByPredicate([Exit](const TWeakObjectPtr<ALB_DungeonExit>& WeakExit)
	{
		return WeakExit.Get() == Exit;
	});
	if (!bBound)
	{
		return;
	}
	// 억제 Pawn이 이 Exit를 벗어났으면 이 Exit의 억제만 해제한다(재진입 시 정상 Nearby로 계산).
	ReleaseExitSuppressions(Exit);
	// 잠금 일괄 갱신 중에는 모든 Exit가 반영된 뒤 호출자가 한 번에 평가한다.
	if (bApplyingExitLock)
	{
		return;
	}
	EvaluateExit(Exit);
}

bool ALB_GameMode::CanEvaluateExits() const
{
	if (!DungeonGameState || bTransitionLocked || bBarrierActive || BoundExitsSerial == INDEX_NONE || bApplyingExitLock)
	{
		return false;
	}
	// 생존 몬스터가 있으면 현재 방 Exit는 잠겨 있다.
	if (HasLivingRoomMonsters())
	{
		return false;
	}
	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	return Snapshot.PendingRoomId.IsNone()
		&& !Snapshot.CurrentRoomId.IsNone()
		&& Snapshot.CurrentRoomId == BoundExitsRoomId
		&& Snapshot.TransitionSerial == BoundExitsSerial
		&& (Snapshot.Phase == ELB_DungeonTransitionPhase::Waiting || Snapshot.Phase == ELB_DungeonTransitionPhase::CountingDown);
}

void ALB_GameMode::EvaluateAllExits()
{
	if (!CanEvaluateExits())
	{
		return;
	}

	// 사라진 Active Exit의 카운트다운은 남기지 않는다.
	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	if (Snapshot.Phase == ELB_DungeonTransitionPhase::CountingDown && !FindBoundExit(Snapshot.ActiveExitId))
	{
		CancelExitCountdown();
	}

	// 평가 중 잠금으로 BoundExits가 비워질 수 있으므로 복사본으로 순회한다.
	TArray<ALB_DungeonExit*> Exits;
	for (const TWeakObjectPtr<ALB_DungeonExit>& WeakExit : BoundExits)
	{
		if (ALB_DungeonExit* Exit = WeakExit.Get())
		{
			Exits.Add(Exit);
		}
	}
	for (ALB_DungeonExit* Exit : Exits)
	{
		if (!CanEvaluateExits())
		{
			break;
		}
		EvaluateExit(Exit);
	}
}

void ALB_GameMode::EvaluateExit(ALB_DungeonExit* Exit)
{
	if (!CanEvaluateExits() || !IsValid(Exit))
	{
		return;
	}

	const FName ExitId = Exit->GetExitId();
	const FLB_DungeonExitLink* Link = FindCurrentExitLink(ExitId);
	if (!Link)
	{
		return;
	}
	// 잠금 중 Route가 바뀌지 않지만 포인터 대신 값으로 들고 간다.
	const FName TargetRoomId = Link->TargetRoomId;
	const FName TargetEntryId = Link->TargetEntryId;

	TArray<APlayerController*> Party;
	GatherParty(Party);
	const int32 Total = Party.Num();
	const int32 Required = Total / 2 + 1;
	const int32 Nearby = CountNearbyParty(Exit, Party);

	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	const int32 RoomSerial = Snapshot.TransitionSerial;
	const bool bCounting = Snapshot.Phase == ELB_DungeonTransitionPhase::CountingDown;
	const bool bIsActive = bCounting && Snapshot.ActiveExitId == ExitId;

	// 1) 전원 진입: 진행 중 카운트다운보다 우선해 즉시 잠근다.
	if (Total > 0 && Nearby == Total)
	{
		LockRoomTransition(ExitId, TargetRoomId, TargetEntryId, RoomSerial);
		return;
	}

	// 2) 엄밀한 과반수: 카운트다운 시작/인원 갱신.
	if (Total > 0 && Nearby >= Required)
	{
		if (bIsActive)
		{
			UpdateExitCountdownCounts(Nearby, Required, Total);
			return;
		}
		// Trigger가 겹친 경우에도 과반수를 유지하는 기존 Active Exit는 재시작하지 않는다.
		if (bCounting)
		{
			if (const ALB_DungeonExit* ActiveExit = FindBoundExit(Snapshot.ActiveExitId))
			{
				if (CountNearbyParty(ActiveExit, Party) >= Required)
				{
					return;
				}
			}
		}
		StartExitCountdown(ExitId, Nearby, Required, Total);
		return;
	}

	// 3) Active Exit가 과반수를 잃으면 취소.
	if (bIsActive)
	{
		CancelExitCountdown();
	}
}

int32 ALB_GameMode::CountNearbyParty(const ALB_DungeonExit* Exit, const TArray<APlayerController*>& Party) const
{
	if (!IsValid(Exit))
	{
		return 0;
	}
	int32 Count = 0;
	for (const APlayerController* PC : Party)
	{
		const APawn* Pawn = PC->GetPawn();
		// 도착 중이거나 이 Exit에서 도착 억제된 Pawn은 집계하지 않는다.
		if (Exit->IsPawnInExit(Pawn) && !IsPawnExcludedFromExit(Exit, Pawn))
		{
			++Count;
		}
	}
	return Count;
}

const FLB_DungeonExitLink* ALB_GameMode::FindCurrentExitLink(FName ExitId) const
{
	if (!DungeonGameState || ExitId.IsNone())
	{
		return nullptr;
	}
	const FLB_DungeonRoomDefinition* Room = DungeonGameState->FindRoomDefinition(DungeonGameState->GetDungeonTransition().CurrentRoomId);
	if (!Room)
	{
		return nullptr;
	}
	return Room->Exits.FindByPredicate([ExitId](const FLB_DungeonExitLink& Link)
	{
		return Link.ExitId == ExitId;
	});
}

FName ALB_GameMode::FindTargetRoomForExit(FName ExitId) const
{
	const FLB_DungeonExitLink* Link = FindCurrentExitLink(ExitId);
	return Link ? Link->TargetRoomId : NAME_None;
}

ALB_DungeonExit* ALB_GameMode::FindBoundExit(FName ExitId) const
{
	if (ExitId.IsNone())
	{
		return nullptr;
	}
	for (const TWeakObjectPtr<ALB_DungeonExit>& WeakExit : BoundExits)
	{
		ALB_DungeonExit* Exit = WeakExit.Get();
		if (IsValid(Exit) && Exit->GetExitId() == ExitId)
		{
			return Exit;
		}
	}
	return nullptr;
}

float ALB_GameMode::GetCountdownDuration() const
{
	return ExitCountdownSeconds > 0.f ? ExitCountdownSeconds : 5.f;
}

void ALB_GameMode::StartExitCountdown(FName ExitId, int32 Nearby, int32 Required, int32 Total)
{
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearTimer(ExitCountdownHandle);

	const float Duration = GetCountdownDuration();
	FLB_DungeonTransitionSnapshot Counting = DungeonGameState->GetDungeonTransition();
	Counting.Phase = ELB_DungeonTransitionPhase::CountingDown;
	Counting.ActiveExitId = ExitId;
	Counting.NearbyPlayers = Nearby;
	Counting.RequiredPlayers = Required;
	Counting.TotalPlayers = Total;
	Counting.CountdownEndServerTime = DungeonGameState->GetServerWorldTimeSeconds() + Duration;
	if (!DungeonGameState->UpdateDungeonTransition(Counting))
	{
		return;
	}

	UE_LOG(LogLB, Log, TEXT("Exit countdown started: %s (%d/%d of %d, %.2fs)."),
		*ExitId.ToString(), Nearby, Required, Total, Duration);
	TimerManager.SetTimer(ExitCountdownHandle,
		FTimerDelegate::CreateUObject(this, &ALB_GameMode::HandleExitCountdownFinished, ExitId, Counting.TransitionSerial),
		Duration, false);
}

void ALB_GameMode::UpdateExitCountdownCounts(int32 Nearby, int32 Required, int32 Total)
{
	const FLB_DungeonTransitionSnapshot& Current = DungeonGameState->GetDungeonTransition();
	if (Current.NearbyPlayers == Nearby && Current.RequiredPlayers == Required && Current.TotalPlayers == Total)
	{
		return;
	}
	// 종료 시간은 유지하고 UI용 인원만 갱신한다.
	FLB_DungeonTransitionSnapshot Updated = Current;
	Updated.NearbyPlayers = Nearby;
	Updated.RequiredPlayers = Required;
	Updated.TotalPlayers = Total;
	DungeonGameState->UpdateDungeonTransition(Updated);
}

void ALB_GameMode::CancelExitCountdown()
{
	GetWorldTimerManager().ClearTimer(ExitCountdownHandle);
	if (!DungeonGameState)
	{
		return;
	}

	FLB_DungeonTransitionSnapshot Waiting = DungeonGameState->GetDungeonTransition();
	UE_LOG(LogLB, Log, TEXT("Exit countdown cancelled: %s."), *Waiting.ActiveExitId.ToString());
	Waiting.Phase = ELB_DungeonTransitionPhase::Waiting;
	Waiting.ActiveExitId = NAME_None;
	Waiting.NearbyPlayers = 0;
	Waiting.RequiredPlayers = 0;
	Waiting.CountdownEndServerTime = 0.0;
	DungeonGameState->UpdateDungeonTransition(Waiting);
}

void ALB_GameMode::HandleExitCountdownFinished(FName ExitId, int32 RoomSerial)
{
	ExitCountdownHandle.Invalidate();
	if (!CanEvaluateExits())
	{
		return;
	}

	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	if (Snapshot.Phase != ELB_DungeonTransitionPhase::CountingDown
		|| Snapshot.ActiveExitId != ExitId
		|| Snapshot.TransitionSerial != RoomSerial)
	{
		return;
	}

	// 완료 시점 인원을 다시 평가한다.
	TArray<APlayerController*> Party;
	GatherParty(Party);
	const int32 Total = Party.Num();
	const int32 Required = Total / 2 + 1;
	const ALB_DungeonExit* Exit = FindBoundExit(ExitId);
	const int32 Nearby = CountNearbyParty(Exit, Party);
	const FLB_DungeonExitLink* Link = FindCurrentExitLink(ExitId);

	if (Exit && Link && Total > 0 && Nearby >= Required)
	{
		const FName TargetRoomId = Link->TargetRoomId;
		const FName TargetEntryId = Link->TargetEntryId;
		LockRoomTransition(ExitId, TargetRoomId, TargetEntryId, RoomSerial);
		return;
	}

	CancelExitCountdown();
	EvaluateAllExits();
}

void ALB_GameMode::LockRoomTransition(FName ExitId, FName TargetRoomId, FName TargetEntryId, int32 RoomSerial)
{
	// TransitionSerial/Phase 가드: 같은 프레임의 Timer 완료·전원 진입 중 하나만 확정한다.
	if (!DungeonGameState || bTransitionLocked || TargetRoomId.IsNone() || TargetEntryId.IsNone())
	{
		return;
	}
	const FLB_DungeonTransitionSnapshot Snapshot = DungeonGameState->GetDungeonTransition();
	if (Snapshot.TransitionSerial != RoomSerial
		|| !Snapshot.PendingRoomId.IsNone()
		|| Snapshot.CurrentRoomId.IsNone()
		|| (Snapshot.Phase != ELB_DungeonTransitionPhase::Waiting && Snapshot.Phase != ELB_DungeonTransitionPhase::CountingDown))
	{
		return;
	}

	bTransitionLocked = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReevaluateHandle);
	}
	// 잠긴 뒤 Exit 이탈은 무시한다(카운트다운 Timer도 여기서 해제).
	UnbindCurrentRoomExits();
	// 출구 밖 인원을 포함한 파티 전체를 캡처한다.
	CaptureTransitionParty();

	FLB_DungeonTransitionSnapshot Next;
	Next.CurrentRoomId = Snapshot.CurrentRoomId;
	Next.CurrentEntryId = Snapshot.CurrentEntryId;
	Next.PendingRoomId = TargetRoomId;
	Next.PendingEntryId = TargetEntryId;
	Next.ActiveExitId = ExitId;
	Next.TransitionSerial = RoomSerial + 1;
	Next.Phase = ELB_DungeonTransitionPhase::LoadingNextRoom;
	Next.TotalPlayers = TransitionParty.Num();
	Next.NearbyPlayers = 0;
	Next.RequiredPlayers = 0;
	Next.CountdownEndServerTime = 0.0;

	UE_LOG(LogLB, Log, TEXT("Room transition locked: %s -[%s]-> %s/%s (serial %d, party %d)."),
		*Snapshot.CurrentRoomId.ToString(), *ExitId.ToString(), *TargetRoomId.ToString(), *TargetEntryId.ToString(),
		Next.TransitionSerial, TransitionParty.Num());

	if (!BeginRoomTransition(Next) && DungeonGameState->GetDungeonTransition().Phase != ELB_DungeonTransitionPhase::Failed)
	{
		FailRoomTransition(TEXT("transition publish failed"));
	}
}

// ============================================================================
// 진입 연출
// ============================================================================

bool ALB_GameMode::ValidateRoomMarkers(ULevel* Level, const FLB_DungeonRoomDefinition& Room, FName TargetEntryId, ALB_DungeonRoomEntry*& OutEntry, FString& OutError)
{
	OutEntry = nullptr;
	if (!Level)
	{
		OutError = TEXT("room level not visible");
		return false;
	}
	if (TargetEntryId.IsNone())
	{
		OutError = TEXT("target EntryId is None");
		return false;
	}

	TMap<FName, ALB_DungeonRoomEntry*> MarkerEntries;
	TSet<FName> MarkerExitIds;
	for (AActor* Actor : Level->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		if (ALB_DungeonRoomEntry* Entry = Cast<ALB_DungeonRoomEntry>(Actor))
		{
			if (!Entry->HasValidEntryId())
			{
				OutError = FString::Printf(TEXT("entry marker %s has invalid EntryId '%s'"), *Entry->GetName(), *Entry->GetEntryId().ToString());
				return false;
			}
			if (MarkerEntries.Contains(Entry->GetEntryId()))
			{
				OutError = FString::Printf(TEXT("duplicate entry marker %s"), *Entry->GetEntryId().ToString());
				return false;
			}
			MarkerEntries.Add(Entry->GetEntryId(), Entry);
			continue;
		}
		if (const ALB_DungeonExit* Exit = Cast<ALB_DungeonExit>(Actor))
		{
			if (!Exit->HasValidExitId())
			{
				OutError = FString::Printf(TEXT("exit marker %s has invalid ExitId"), *Exit->GetName());
				return false;
			}
			bool bDuplicate = false;
			MarkerExitIds.Add(Exit->GetExitId(), &bDuplicate);
			if (bDuplicate)
			{
				OutError = FString::Printf(TEXT("duplicate exit marker %s"), *Exit->GetExitId().ToString());
				return false;
			}
		}
	}

	// Entry Marker 집합은 RoomDefinition.EntryIds와 정확히 일치해야 한다.
	TSet<FName> DeclaredEntryIds;
	DeclaredEntryIds.Append(Room.EntryIds);
	TSet<FName> FoundEntryIds;
	MarkerEntries.GetKeys(FoundEntryIds);
	if (DeclaredEntryIds.Num() != FoundEntryIds.Num() || !DeclaredEntryIds.Includes(FoundEntryIds))
	{
		OutError = FString::Printf(TEXT("entry markers (%d) do not match declared EntryIds (%d)"), FoundEntryIds.Num(), DeclaredEntryIds.Num());
		return false;
	}

	TSet<FName> GraphExitIds;
	for (const FLB_DungeonExitLink& Link : Room.Exits)
	{
		GraphExitIds.Add(Link.ExitId);
	}
	if (GraphExitIds.Num() != MarkerExitIds.Num() || !GraphExitIds.Includes(MarkerExitIds))
	{
		OutError = FString::Printf(TEXT("exit markers (%d) do not match graph exits (%d)"), MarkerExitIds.Num(), GraphExitIds.Num());
		return false;
	}

	// 선택 Entry는 중복 거부 후 Map에서 정확히 하나로 결정된다.
	ALB_DungeonRoomEntry* const* Selected = MarkerEntries.Find(TargetEntryId);
	if (!Selected || !IsValid(*Selected))
	{
		OutError = FString::Printf(TEXT("target entry marker %s not found"), *TargetEntryId.ToString());
		return false;
	}

	OutEntry = *Selected;
	return true;
}

void ALB_GameMode::BroadcastDungeonFade(bool bFadeOut, float Duration)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALB_PlayerController* PC = Cast<ALB_PlayerController>(It->Get()))
		{
			PC->ClientSetDungeonFade(bFadeOut, Duration);
		}
	}
}

void ALB_GameMode::ScheduleTransitionStep(float Delay, void (ALB_GameMode::*Step)(int32), int32 Serial)
{
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearTimer(TransitionStepHandle);
	const FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, Step, Serial);
	if (Delay > 0.f)
	{
		TimerManager.SetTimer(TransitionStepHandle, Delegate, Delay, false);
	}
	else
	{
		TransitionStepHandle = TimerManager.SetTimerForNextTick(Delegate);
	}
}

void ALB_GameMode::HandleFadeOutFinished(int32 Serial)
{
	TransitionStepHandle.Invalidate();
	if (!DungeonGameState)
	{
		return;
	}
	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	if (Snapshot.Phase != ELB_DungeonTransitionPhase::FadingOut
		|| Snapshot.TransitionSerial != Serial
		|| Snapshot.PendingRoomId.IsNone())
	{
		return;
	}

	ALB_DungeonRoomEntry* Entry = PendingEntry.Get();
	if (!IsValid(Entry) || Entry->GetEntryId() != Snapshot.PendingEntryId)
	{
		FailRoomTransition(TEXT("room entry lost"));
		return;
	}

	// 0) 이전 Current Runtime 정리 → Pending 방 Runtime 활성화 + Exit 잠금 반영.
	//    실패하면 Teleport·승격 없이 Pending Runtime을 롤백하고 Fade 복구 후 Failed로 남긴다.
	const FName PendingRoomId = Snapshot.PendingRoomId;
	FString RuntimeError;
	if (!ActivatePendingRoomRuntime(PendingRoomId, RuntimeError))
	{
		UE_LOG(LogLB, Error, TEXT("Room %s (serial %d) runtime activation failed: %s"),
			*PendingRoomId.ToString(), Serial, *RuntimeError);
		FailRoomTransition(TEXT("room runtime activation failed"));
		return;
	}

	// 1) 서버가 캡처된 파티 전체를 선택 Entry로 Teleport.
	TeleportPartyToEntry(Entry);
	PendingEntry.Reset();

	// 이전 방 Entry는 곧 해제되므로 먼저 버린다.
	CurrentEntry.Reset();
	CurrentEntryRoomId = NAME_None;
	CurrentEntryMarkerId = NAME_None;
	CurrentEntrySerial = INDEX_NONE;

	// 2) Pending 승격 + 이전 방 해제.
	if (!DungeonGameState->CompleteDungeonTransition())
	{
		FailRoomTransition(TEXT("promote failed"));
		return;
	}

	// 승격된 방 Entry를 안정 상태 Entry로 보관한다(늦은 합류용).
	{
		const FLB_DungeonTransitionSnapshot& Promoted = DungeonGameState->GetDungeonTransition();
		CurrentEntry = Entry;
		CurrentEntryRoomId = Promoted.CurrentRoomId;
		CurrentEntryMarkerId = Promoted.CurrentEntryId;
		CurrentEntrySerial = Promoted.TransitionSerial;
	}

	// 3) Fade In.
	FLB_DungeonTransitionSnapshot FadingIn = DungeonGameState->GetDungeonTransition();
	FadingIn.Phase = ELB_DungeonTransitionPhase::FadingIn;
	FadingIn.ActiveExitId = NAME_None;
	FadingIn.NearbyPlayers = 0;
	FadingIn.RequiredPlayers = 0;
	FadingIn.CountdownEndServerTime = 0.0;
	if (!DungeonGameState->UpdateDungeonTransition(FadingIn))
	{
		FailRoomTransition(TEXT("fade in publish failed"));
		return;
	}

	BroadcastDungeonFade(false, FadeInDuration);
	ScheduleTransitionStep(FadeInDuration, &ALB_GameMode::HandleFadeInFinished, FadingIn.TransitionSerial);
}

void ALB_GameMode::HandleFadeInFinished(int32 Serial)
{
	TransitionStepHandle.Invalidate();
	if (!DungeonGameState)
	{
		return;
	}
	const FLB_DungeonTransitionSnapshot& Snapshot = DungeonGameState->GetDungeonTransition();
	if (Snapshot.Phase != ELB_DungeonTransitionPhase::FadingIn
		|| Snapshot.TransitionSerial != Serial
		|| !Snapshot.PendingRoomId.IsNone())
	{
		return;
	}

	TArray<APlayerController*> Party;
	GatherParty(Party);

	FLB_DungeonTransitionSnapshot Stable = Snapshot;
	Stable.Phase = ELB_DungeonTransitionPhase::Waiting;
	Stable.ActiveExitId = NAME_None;
	Stable.TotalPlayers = Party.Num();
	Stable.NearbyPlayers = 0;
	Stable.RequiredPlayers = 0;
	Stable.CountdownEndServerTime = 0.0;
	if (!DungeonGameState->UpdateDungeonTransition(Stable))
	{
		FailRoomTransition(TEXT("stable publish failed"));
		return;
	}

	bTransitionLocked = false;
	TransitionParty.Reset();

	UE_LOG(LogLB, Log, TEXT("Room %s entered (serial %d)."), *Stable.CurrentRoomId.ToString(), Stable.TransitionSerial);
	BindCurrentRoomExits();
	EvaluateAllExits();
}

void ALB_GameMode::TeleportPartyToEntry(const ALB_DungeonRoomEntry* Entry)
{
	TArray<APlayerController*> Movers;
	for (const TWeakObjectPtr<APlayerController>& WeakPC : TransitionParty)
	{
		APlayerController* PC = WeakPC.Get();
		if (IsValid(PC) && IsValid(PC->GetPawn()))
		{
			Movers.Add(PC);
		}
	}

	// 새 방 기준으로 배치 완료 집합을 다시 만든다(전환 중 접속자는 포함되지 않음 → 안정 후 늦은 합류).
	CurrentRoomPlaced.Reset();

	// Teleport 전에 모든 이동 Pawn을 도착 중으로 표시한다(Overlap callback 경쟁 방지).
	for (APlayerController* PC : Movers)
	{
		MarkArrivingPawn(PC->GetPawn());
	}

	// Entry 로컬 Y축을 따라 중앙 정렬로 간격 배치한다.
	const int32 Count = Movers.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		APlayerController* PC = Movers[Index];
		APawn* Pawn = PC->GetPawn();
		const float OffsetY = (static_cast<float>(Index) - (Count - 1) * 0.5f) * PartyEntrySpacing;
		if (TeleportControllerToEntry(PC, Entry, OffsetY))
		{
			CurrentRoomPlaced.Add(PC);
			// 도착 직후 실제로 겹친 Exit에서만 억제한다.
			FinishPawnArrival(Pawn, Entry);
		}
		else
		{
			ArrivingPawns.Remove(Pawn);
		}
	}
	UE_LOG(LogLB, Log, TEXT("Party teleported to entry %s: %d pawns."), *Entry->GetEntryId().ToString(), Count);
}

// ============================================================================
// 도착 Exit 억제
// ============================================================================

void ALB_GameMode::MarkArrivingPawn(APawn* Pawn)
{
	if (IsValid(Pawn))
	{
		ArrivingPawns.Add(Pawn);
	}
}

void ALB_GameMode::FinishPawnArrival(APawn* Pawn, const ALB_DungeonRoomEntry* Entry)
{
	ArrivingPawns.Remove(Pawn);
	if (!IsValid(Pawn) || !IsValid(Entry))
	{
		return;
	}

	ULevel* Level = Entry->GetLevel();
	if (!Level)
	{
		return;
	}

	// 도착 Level의 Exit 중 Pawn이 실제로 겹친 Exit에만 Exit별로 독립 억제를 건다.
	for (AActor* Actor : Level->Actors)
	{
		ALB_DungeonExit* Exit = Cast<ALB_DungeonExit>(Actor);
		if (IsValid(Exit) && Exit->IsPawnInExit(Pawn))
		{
			SuppressedExitPawns.FindOrAdd(Exit).Add(Pawn);
			UE_LOG(LogLB, Log, TEXT("Arrival suppression: %s in exit %s."), *Pawn->GetName(), *Exit->GetExitId().ToString());
		}
	}
}

void ALB_GameMode::ReleaseExitSuppressions(ALB_DungeonExit* Exit)
{
	if (!IsValid(Exit))
	{
		return;
	}
	TSet<TWeakObjectPtr<APawn>>* Suppressed = SuppressedExitPawns.Find(Exit);
	if (!Suppressed)
	{
		return;
	}

	for (auto It = Suppressed->CreateIterator(); It; ++It)
	{
		APawn* Pawn = It->Get();
		// 도착 처리 중인 Pawn은 FinishPawnArrival에서 결정한다.
		if (!IsValid(Pawn) || (!Exit->IsPawnInExit(Pawn) && !ArrivingPawns.Contains(*It)))
		{
			if (IsValid(Pawn))
			{
				UE_LOG(LogLB, Log, TEXT("Arrival suppression released: %s left exit %s."), *Pawn->GetName(), *Exit->GetExitId().ToString());
			}
			It.RemoveCurrent();
		}
	}
	if (Suppressed->IsEmpty())
	{
		SuppressedExitPawns.Remove(Exit);
	}
}

bool ALB_GameMode::IsPawnExcludedFromExit(const ALB_DungeonExit* Exit, const APawn* Pawn) const
{
	if (!Pawn)
	{
		return false;
	}
	const TWeakObjectPtr<APawn> PawnKey(const_cast<APawn*>(Pawn));
	if (ArrivingPawns.Contains(PawnKey))
	{
		return true;
	}
	const TSet<TWeakObjectPtr<APawn>>* Suppressed = SuppressedExitPawns.Find(TWeakObjectPtr<ALB_DungeonExit>(const_cast<ALB_DungeonExit*>(Exit)));
	return Suppressed && Suppressed->Contains(PawnKey);
}

void ALB_GameMode::RemovePawnSuppressions(const APawn* Pawn)
{
	if (Pawn)
	{
		const TWeakObjectPtr<APawn> PawnKey(const_cast<APawn*>(Pawn));
		ArrivingPawns.Remove(PawnKey);
		for (TPair<TWeakObjectPtr<ALB_DungeonExit>, TSet<TWeakObjectPtr<APawn>>>& Pair : SuppressedExitPawns)
		{
			Pair.Value.Remove(PawnKey);
		}
	}
	PruneExitSuppressions();
}

void ALB_GameMode::PruneExitSuppressions()
{
	for (auto It = ArrivingPawns.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
	for (auto MapIt = SuppressedExitPawns.CreateIterator(); MapIt; ++MapIt)
	{
		if (!MapIt->Key.IsValid())
		{
			MapIt.RemoveCurrent();
			continue;
		}
		for (auto SetIt = MapIt->Value.CreateIterator(); SetIt; ++SetIt)
		{
			if (!SetIt->IsValid())
			{
				SetIt.RemoveCurrent();
			}
		}
		if (MapIt->Value.IsEmpty())
		{
			MapIt.RemoveCurrent();
		}
	}
}

bool ALB_GameMode::TeleportControllerToEntry(APlayerController* PC, const ALB_DungeonRoomEntry* Entry, float OffsetY)
{
	APawn* Pawn = IsValid(PC) ? PC->GetPawn() : nullptr;
	if (!IsValid(Pawn) || !IsValid(Entry))
	{
		return false;
	}

	const FTransform EntryTransform = Entry->GetEntryTransform();
	const FRotator EntryRotation = Entry->GetEntryRotation();
	const FVector Location = EntryTransform.TransformPosition(FVector(0.f, OffsetY, 0.f));

	if (!Pawn->TeleportTo(Location, EntryRotation))
	{
		if (!Pawn->TeleportTo(Location, EntryRotation, false, true))
		{
			UE_LOG(LogLB, Warning, TEXT("TeleportControllerToEntry: failed to teleport %s."), *Pawn->GetName());
			return false;
		}
	}
	PC->SetControlRotation(EntryRotation);
	PC->ClientSetRotation(EntryRotation, true);
	return true;
}

void ALB_GameMode::ClearTransitionTimers()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(ExitCountdownHandle);
		TimerManager.ClearTimer(TransitionStepHandle);
	}
}

// ============================================================================
// 방 Runtime
// ============================================================================

bool ALB_GameMode::CreateRoomRuntimeManager(FName InstanceKey)
{
	DestroyRoomRuntimeManager();

	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || InstanceKey.IsNone())
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	ALB_DungeonRoomRuntimeManager* Manager = World->SpawnActor<ALB_DungeonRoomRuntimeManager>(
		ALB_DungeonRoomRuntimeManager::StaticClass(), FTransform::Identity, Params);
	if (!IsValid(Manager))
	{
		UE_LOG(LogLB, Error, TEXT("CreateRoomRuntimeManager: spawn failed (instance %s)."), *InstanceKey.ToString());
		return false;
	}

	Manager->SetSessionInstanceKey(InstanceKey);
	RoomRuntimeManager = Manager;
	RoomCombatStateHandle = Manager->OnRoomCombatStateChanged().AddUObject(this, &ALB_GameMode::HandleRoomCombatStateChanged);
	UE_LOG(LogLB, Log, TEXT("Room runtime manager created (instance %s)."), *InstanceKey.ToString());
	return true;
}

void ALB_GameMode::DestroyRoomRuntimeManager()
{
	ALB_DungeonRoomRuntimeManager* Manager = RoomRuntimeManager.Get();
	RoomRuntimeManager = nullptr;
	RuntimeRoomLevel.Reset();
	if (!IsValid(Manager))
	{
		RoomCombatStateHandle.Reset();
		return;
	}

	Manager->OnRoomCombatStateChanged().Remove(RoomCombatStateHandle);
	RoomCombatStateHandle.Reset();
	Manager->DeactivateRoom();

	const UWorld* World = GetWorld();
	if (World && !World->bIsTearingDown)
	{
		Manager->Destroy();
	}
}

void ALB_GameMode::DeactivateRoomRuntime()
{
	RuntimeRoomLevel.Reset();
	if (ALB_DungeonRoomRuntimeManager* Manager = RoomRuntimeManager.Get(); IsValid(Manager) && Manager->IsRoomActive())
	{
		UE_LOG(LogLB, Log, TEXT("Room runtime deactivated: %s."), *Manager->GetActiveRoomId().ToString());
		Manager->DeactivateRoom();
	}
}

bool ALB_GameMode::ActivatePendingRoomRuntime(FName RoomId, FString& OutError)
{
	ALB_DungeonRoomRuntimeManager* Manager = RoomRuntimeManager.Get();
	if (!IsValid(Manager) || !DungeonGameState)
	{
		OutError = TEXT("room runtime manager missing");
		return false;
	}
	if (Manager->GetSessionInstanceKey() != DungeonGameState->GetDungeonRoute().InstanceKey)
	{
		OutError = TEXT("room runtime session mismatch");
		return false;
	}

	// 이전 Current 방 Loader가 CompleteDungeonTransition에서 해제되기 전에 관리 Actor를 먼저 정리한다.
	DeactivateRoomRuntime();

	ULevel* Level = DungeonGameState->GetPendingRoomLevel();
	if (!Level || RoomId.IsNone())
	{
		OutError = TEXT("pending room level not visible");
		return false;
	}

	// 활성화 성공 통지(HandleRoomCombatStateChanged)가 이 Level의 Exit에 잠금을 반영할 수 있도록 먼저 지정한다.
	RuntimeRoomLevel = Level;
	if (!Manager->ActivateRoom(RoomId, Level))
	{
		// Manager가 이번 시도의 Actor를 롤백한다. 남은 활성 참조도 정리한다.
		DeactivateRoomRuntime();
		OutError = TEXT("ActivateRoom failed (markers or spawn)");
		return false;
	}

	const bool bLocked = Manager->HasLivingMonsters();
	ApplyRoomExitLock(Level, bLocked);
	UE_LOG(LogLB, Log, TEXT("Room runtime activated: %s (living monsters %d, exits %s)."),
		*RoomId.ToString(), Manager->GetLivingMonsterCount(), bLocked ? TEXT("locked") : TEXT("open"));
	return true;
}

void ALB_GameMode::ApplyRoomExitLock(ULevel* Level, bool bLocked)
{
	if (!Level)
	{
		return;
	}
	TArray<ALB_DungeonExit*> Exits;
	for (AActor* Actor : Level->Actors)
	{
		if (ALB_DungeonExit* Exit = Cast<ALB_DungeonExit>(Actor); IsValid(Exit))
		{
			Exits.Add(Exit);
		}
	}

	TGuardValue<bool> Guard(bApplyingExitLock, true);
	for (ALB_DungeonExit* Exit : Exits)
	{
		if (IsValid(Exit))
		{
			Exit->SetExitEnabled(!bLocked);
		}
	}
}

void ALB_GameMode::HandleRoomCombatStateChanged(FName RoomId, int32 LivingMonsterCount)
{
	const ALB_DungeonRoomRuntimeManager* Manager = RoomRuntimeManager.Get();
	if (!IsValid(Manager) || RoomId.IsNone() || Manager->GetActiveRoomId() != RoomId)
	{
		return;
	}
	ULevel* Level = RuntimeRoomLevel.Get();
	if (!Level)
	{
		return;
	}

	const bool bLocked = LivingMonsterCount > 0;
	ApplyRoomExitLock(Level, bLocked);
	if (bLocked)
	{
		return;
	}

	UE_LOG(LogLB, Log, TEXT("Room %s cleared: exits unlocked."), *RoomId.ToString());
	// 전환 잠금 중에는 CanEvaluateExits가 막아 목적지·Barrier를 바꾸지 않는다.
	// 안정 상태라면 이미 영역 안에 있던 Pawn을 포함해 Exit별 과반수 평가를 재개한다.
	EvaluateAllExits();
}

bool ALB_GameMode::HasLivingRoomMonsters() const
{
	const ALB_DungeonRoomRuntimeManager* Manager = RoomRuntimeManager.Get();
	return IsValid(Manager) && Manager->IsRoomActive() && Manager->HasLivingMonsters();
}
