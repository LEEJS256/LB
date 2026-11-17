// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LB_GameMode.h"

#include "LB.h"
#include "GameMode/LB_GameState.h"
#include "Controller/LB_PlayerController.h"
#include "Data/Dungeon/LB_DungeonLayoutDataAsset.h"
#include "Dungeon/LB_DungeonRoomMarkers.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Misc/Guid.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Level.h"

void ALB_GameMode::StartPlay()
{
	// 기본 StartPlay에서 매치가 시작되며 Pawn이 스폰되므로 Barrier 참가자 캡처 전에 호출한다.
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

	if (!StartupDungeonLayout)
	{
		return;
	}

	if (DungeonGameState->HasDungeonRoute())
	{
		UE_LOG(LogLB, Warning, TEXT("ALB_GameMode::StartPlay: Route already published."));
		return;
	}

	const FName InstanceKey(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
	FLB_DungeonRouteState NewRoute;
	FString Error;
	if (!StartupDungeonLayout->BuildRoute(InstanceKey, NewRoute, Error))
	{
		UE_LOG(LogLB, Error, TEXT("ALB_GameMode::StartPlay: BuildRoute failed: %s"), *Error);
		return;
	}

	if (!DungeonGameState->PublishDungeonRoute(NewRoute))
	{
		UE_LOG(LogLB, Error, TEXT("ALB_GameMode::StartPlay: PublishDungeonRoute failed."));
		return;
	}

	// StartRoom 진입도 Exit 전환과 같은 잠금·Fade·Teleport 경로를 사용한다.
	CaptureTransitionParty();
	bTransitionLocked = true;

	FLB_DungeonTransitionSnapshot Snapshot;
	Snapshot.CurrentRoomId = NAME_None;
	Snapshot.PendingRoomId = NewRoute.StartRoomId;
	Snapshot.ActiveExitId = NAME_None;
	Snapshot.TransitionSerial = 1;
	Snapshot.Phase = ELB_DungeonTransitionPhase::LoadingNextRoom;
	Snapshot.TotalPlayers = TransitionParty.Num();
	if (!BeginRoomTransition(Snapshot) && DungeonGameState->GetDungeonTransition().Phase != ELB_DungeonTransitionPhase::Failed)
	{
		FailRoomTransition(TEXT("start room publish failed"));
	}
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
	bTransitionLocked = false;
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
	if (!Room || !ValidateRoomMarkers(DungeonGameState->GetPendingRoomLevel(), *Room, Entry, Error))
	{
		UE_LOG(LogLB, Error, TEXT("Room %s (serial %d) markers invalid: %s"),
			*Snapshot.PendingRoomId.ToString(), Snapshot.TransitionSerial, *Error);
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
}

void ALB_GameMode::UnbindGameStateDelegates()
{
	if (DungeonGameState)
	{
		DungeonGameState->OnServerPendingRoomReady.Remove(ServerReadyHandle);
		DungeonGameState->OnClientPendingRoomReady.Remove(ClientReadyHandle);
		DungeonGameState->OnLocalPendingRoomFailed.Remove(LocalFailedHandle);
	}
	ServerReadyHandle.Reset();
	ClientReadyHandle.Reset();
	LocalFailedHandle.Reset();
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
	}
	BoundExits.Reset();
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
	EvaluateExit(Exit);
}

bool ALB_GameMode::CanEvaluateExits() const
{
	if (!DungeonGameState || bTransitionLocked || bBarrierActive || BoundExitsSerial == INDEX_NONE)
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
	const FName TargetRoomId = FindTargetRoomForExit(ExitId);
	if (TargetRoomId.IsNone())
	{
		return;
	}

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
		LockRoomTransition(ExitId, TargetRoomId, RoomSerial);
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
		if (Exit->IsPawnInExit(PC->GetPawn()))
		{
			++Count;
		}
	}
	return Count;
}

FName ALB_GameMode::FindTargetRoomForExit(FName ExitId) const
{
	if (!DungeonGameState || ExitId.IsNone())
	{
		return NAME_None;
	}
	const FLB_DungeonRoomDefinition* Room = DungeonGameState->FindRoomDefinition(DungeonGameState->GetDungeonTransition().CurrentRoomId);
	if (!Room)
	{
		return NAME_None;
	}
	for (const FLB_DungeonExitLink& Link : Room->Exits)
	{
		if (Link.ExitId == ExitId)
		{
			return Link.TargetRoomId;
		}
	}
	return NAME_None;
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
	const FName TargetRoomId = FindTargetRoomForExit(ExitId);

	if (Exit && !TargetRoomId.IsNone() && Total > 0 && Nearby >= Required)
	{
		LockRoomTransition(ExitId, TargetRoomId, RoomSerial);
		return;
	}

	CancelExitCountdown();
	EvaluateAllExits();
}

void ALB_GameMode::LockRoomTransition(FName ExitId, FName TargetRoomId, int32 RoomSerial)
{
	// TransitionSerial/Phase 가드: 같은 프레임의 Timer 완료·전원 진입 중 하나만 확정한다.
	if (!DungeonGameState || bTransitionLocked || TargetRoomId.IsNone())
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
	Next.PendingRoomId = TargetRoomId;
	Next.ActiveExitId = ExitId;
	Next.TransitionSerial = RoomSerial + 1;
	Next.Phase = ELB_DungeonTransitionPhase::LoadingNextRoom;
	Next.TotalPlayers = TransitionParty.Num();
	Next.NearbyPlayers = 0;
	Next.RequiredPlayers = 0;
	Next.CountdownEndServerTime = 0.0;

	UE_LOG(LogLB, Log, TEXT("Room transition locked: %s -[%s]-> %s (serial %d, party %d)."),
		*Snapshot.CurrentRoomId.ToString(), *ExitId.ToString(), *TargetRoomId.ToString(), Next.TransitionSerial, TransitionParty.Num());

	if (!BeginRoomTransition(Next) && DungeonGameState->GetDungeonTransition().Phase != ELB_DungeonTransitionPhase::Failed)
	{
		FailRoomTransition(TEXT("transition publish failed"));
	}
}

// ============================================================================
// 진입 연출
// ============================================================================

bool ALB_GameMode::ValidateRoomMarkers(ULevel* Level, const FLB_DungeonRoomDefinition& Room, ALB_DungeonRoomEntry*& OutEntry, FString& OutError)
{
	OutEntry = nullptr;
	if (!Level)
	{
		OutError = TEXT("room level not visible");
		return false;
	}

	int32 EntryCount = 0;
	ALB_DungeonRoomEntry* FoundEntry = nullptr;
	TSet<FName> MarkerExitIds;
	for (AActor* Actor : Level->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		if (ALB_DungeonRoomEntry* Entry = Cast<ALB_DungeonRoomEntry>(Actor))
		{
			++EntryCount;
			FoundEntry = Entry;
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

	if (EntryCount != 1)
	{
		OutError = FString::Printf(TEXT("expected exactly one RoomEntry, found %d"), EntryCount);
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

	OutEntry = FoundEntry;
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
	if (!IsValid(Entry))
	{
		FailRoomTransition(TEXT("room entry lost"));
		return;
	}

	// 1) 서버가 캡처된 파티 전체를 Entry로 Teleport.
	TeleportPartyToEntry(Entry);
	PendingEntry.Reset();

	// 2) Pending 승격 + 이전 방 해제.
	if (!DungeonGameState->CompleteDungeonTransition())
	{
		FailRoomTransition(TEXT("promote failed"));
		return;
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
	const FTransform EntryTransform = Entry->GetEntryTransform();
	const FRotator EntryRotation = Entry->GetEntryRotation();

	TArray<APlayerController*> Movers;
	for (const TWeakObjectPtr<APlayerController>& WeakPC : TransitionParty)
	{
		APlayerController* PC = WeakPC.Get();
		if (IsValid(PC) && IsValid(PC->GetPawn()))
		{
			Movers.Add(PC);
		}
	}

	// Entry 로컬 Y축을 따라 중앙 정렬로 간격 배치한다.
	const int32 Count = Movers.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		APlayerController* PC = Movers[Index];
		APawn* Pawn = PC->GetPawn();
		const float OffsetY = (static_cast<float>(Index) - (Count - 1) * 0.5f) * PartyEntrySpacing;
		const FVector Location = EntryTransform.TransformPosition(FVector(0.f, OffsetY, 0.f));

		if (!Pawn->TeleportTo(Location, EntryRotation))
		{
			if (!Pawn->TeleportTo(Location, EntryRotation, false, true))
			{
				UE_LOG(LogLB, Warning, TEXT("TeleportPartyToEntry: failed to teleport %s."), *Pawn->GetName());
				continue;
			}
		}
		PC->SetControlRotation(EntryRotation);
		PC->ClientSetRotation(EntryRotation, true);
	}
	UE_LOG(LogLB, Log, TEXT("Party teleported to entry: %d pawns."), Count);
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
