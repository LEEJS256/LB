// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LB_GameMode.h"

#include "LB.h"
#include "GameMode/LB_GameState.h"
#include "Controller/LB_PlayerController.h"
#include "Data/Dungeon/LB_DungeonLayoutDataAsset.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Misc/Guid.h"
#include "TimerManager.h"
#include "Engine/World.h"

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

	FLB_DungeonTransitionSnapshot Snapshot;
	Snapshot.CurrentRoomId = NAME_None;
	Snapshot.PendingRoomId = NewRoute.StartRoomId;
	Snapshot.ActiveExitId = NAME_None;
	Snapshot.TransitionSerial = 1;
	Snapshot.Phase = ELB_DungeonTransitionPhase::LoadingNextRoom;
	BeginRoomTransition(Snapshot);
}

void ALB_GameMode::PostLogin(APlayerController* NewPlayer)
{
	// 전환 중 접속한 Controller는 현재 Barrier에 넣지 않는다.
	// 안정된 Current 방은 GameState 복제로 로드된다.
	Super::PostLogin(NewPlayer);
}

void ALB_GameMode::Logout(AController* Exiting)
{
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

	// 최초 시작 방: 이전 방이 없으므로 Fade 없이 Current로 승격하고 대기 상태로 둔다.
	// 방 간 전환의 Fade·Teleport·승격은 후속 작업에서 이 경로에 연결한다.
	if (Snapshot.CurrentRoomId.IsNone())
	{
		if (DungeonGameState->CompleteDungeonTransition())
		{
			FLB_DungeonTransitionSnapshot Stable = DungeonGameState->GetDungeonTransition();
			Stable.Phase = ELB_DungeonTransitionPhase::Waiting;
			DungeonGameState->UpdateDungeonTransition(Stable);
		}
	}
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

	// Fade·Teleport·자동 재시도 없이 실패 상태만 남긴다.
	if (DungeonGameState)
	{
		FLB_DungeonTransitionSnapshot Failed = DungeonGameState->GetDungeonTransition();
		Failed.Phase = ELB_DungeonTransitionPhase::Failed;
		DungeonGameState->UpdateDungeonTransition(Failed);
	}
}
