// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon/LB_DungeonRoomLoader.h"

#include "LB.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"

ALB_DungeonRoomLoader::ALB_DungeonRoomLoader()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;
	SetReplicateMovement(false);
}

FString ALB_DungeonRoomLoader::MakeRoomLevelName(FName InstanceKey, int32 TransitionSerial, FName RoomId)
{
	return FString::Printf(TEXT("LB_%s_%d_%s"), *InstanceKey.ToString(), TransitionSerial, *RoomId.ToString());
}

bool ALB_DungeonRoomLoader::IsSameRequest(FName InstanceKey, int32 TransitionSerial, const FLB_DungeonRoomDefinition& Room, const FTransform& SlotTransform) const
{
	return RequestedInstanceKey == InstanceKey
		&& RequestedTransitionSerial == TransitionSerial
		&& RequestedRoomId == Room.RoomId
		&& RequestedLevelPath == Room.Level.ToSoftObjectPath()
		&& RequestedSlotTransform.Equals(SlotTransform);
}

bool ALB_DungeonRoomLoader::LoadRoomLocal(FName InstanceKey, int32 TransitionSerial, const FLB_DungeonRoomDefinition& Room, const FTransform& SlotTransform)
{
	switch (LoaderState)
	{
	case ELB_DungeonRoomLoaderState::Loading:
	case ELB_DungeonRoomLoaderState::Ready:
		if (IsSameRequest(InstanceKey, TransitionSerial, Room, SlotTransform))
		{
			// 멱등: 중복 로드 없이 기존 수락 결과 반환.
			return bRequestAccepted;
		}
		UE_LOG(LogLB, Warning, TEXT("[RoomLoader] %s: 다른 방 요청 거부 (현재 %s, 요청 %s)"),
			*GetName(), *RequestedLevelName, *MakeRoomLevelName(InstanceKey, TransitionSerial, Room.RoomId));
		return false;

	case ELB_DungeonRoomLoaderState::Failed:
	case ELB_DungeonRoomLoaderState::Unloading:
		UE_LOG(LogLB, Warning, TEXT("[RoomLoader] %s: 실패/해제된 로더 재사용 거부"), *GetName());
		return false;

	case ELB_DungeonRoomLoaderState::Idle:
	default:
		break;
	}

	// 입력 검증. 수락 전 거부이므로 상태와 이벤트를 바꾸지 않는다.
	if (!FLB_DungeonRouteState::IsValidIdentifier(InstanceKey))
	{
		UE_LOG(LogLB, Warning, TEXT("[RoomLoader] %s: 잘못된 InstanceKey '%s'"), *GetName(), *InstanceKey.ToString());
		return false;
	}
	if (TransitionSerial <= 0)
	{
		UE_LOG(LogLB, Warning, TEXT("[RoomLoader] %s: TransitionSerial은 양수여야 함 (%d)"), *GetName(), TransitionSerial);
		return false;
	}
	if (!FLB_DungeonRouteState::IsValidIdentifier(Room.RoomId))
	{
		UE_LOG(LogLB, Warning, TEXT("[RoomLoader] %s: 잘못된 RoomId '%s'"), *GetName(), *Room.RoomId.ToString());
		return false;
	}
	if (Room.Level.IsNull())
	{
		UE_LOG(LogLB, Warning, TEXT("[RoomLoader] %s: Room '%s'의 Level이 비어 있음"), *GetName(), *Room.RoomId.ToString());
		return false;
	}

	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || IsActorBeingDestroyed() || HasActorBegunPlay() == false)
	{
		UE_LOG(LogLB, Warning, TEXT("[RoomLoader] %s: 유효한 World/BeginPlay 이후에만 로드 가능"), *GetName());
		return false;
	}

	RequestedInstanceKey = InstanceKey;
	RequestedTransitionSerial = TransitionSerial;
	RequestedRoomId = Room.RoomId;
	RequestedLevelPath = Room.Level.ToSoftObjectPath();
	RequestedSlotTransform = SlotTransform;
	RequestedLevelName = MakeRoomLevelName(InstanceKey, TransitionSerial, Room.RoomId);

	// 로더 Actor Transform은 합성하지 않고 슬롯 Transform을 그대로 사용한다.
	bool bSuccess = false;
	ULevelStreamingDynamic* Streaming = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		this, Room.Level, SlotTransform, bSuccess, RequestedLevelName);

	if (!bSuccess || !Streaming)
	{
		LevelStreaming = Streaming;
		bRequestAccepted = false;
		FailRequest(TEXT("LoadLevelInstanceBySoftObjectPtr 실패"));
		return false;
	}

	LevelStreaming = Streaming;
	bRequestAccepted = true;
	LoaderState = ELB_DungeonRoomLoaderState::Loading;

	LevelStreaming->OnLevelShown.AddDynamic(this, &ALB_DungeonRoomLoader::HandleLevelShown);
	bOnLevelShownBound = true;

	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.SetTimer(TimeoutHandle, this, &ALB_DungeonRoomLoader::HandleLoadTimeout,
		FMath::Max(LoadTimeoutSeconds, 0.1f), false);

	// 이미 표시된 경우를 위해 현재 상태를 다음 Tick에 확인한다(호출자 바인딩 이후 발행).
	ReadyCheckHandle = TimerManager.SetTimerForNextTick(this, &ALB_DungeonRoomLoader::CheckReadyState);

	UE_LOG(LogLB, Log, TEXT("[RoomLoader] %s: 로드 요청 수락 %s"), *GetName(), *RequestedLevelName);
	return true;
}

void ALB_DungeonRoomLoader::HandleLevelShown()
{
	CheckReadyState();
}

void ALB_DungeonRoomLoader::CheckReadyState()
{
	ReadyCheckHandle.Invalidate();

	if (LoaderState != ELB_DungeonRoomLoaderState::Loading || !LevelStreaming)
	{
		return;
	}

	const ULevel* Level = LevelStreaming->GetLoadedLevel();
	if (Level && Level->bIsVisible && LevelStreaming->IsLevelVisible())
	{
		CompleteReady();
	}
}

void ALB_DungeonRoomLoader::HandleLoadTimeout()
{
	TimeoutHandle.Invalidate();

	if (LoaderState != ELB_DungeonRoomLoaderState::Loading)
	{
		return;
	}
	FailRequest(TEXT("Timeout"));
}

void ALB_DungeonRoomLoader::CompleteReady()
{
	ClearPendingCallbacks();
	LoaderState = ELB_DungeonRoomLoaderState::Ready;

	UE_LOG(LogLB, Log, TEXT("[RoomLoader] %s: 방 표시 완료 %s"), *GetName(), *RequestedLevelName);
	OnRoomReady.Broadcast(this);
}

void ALB_DungeonRoomLoader::FailRequest(const TCHAR* Reason)
{
	ClearPendingCallbacks();
	ReleaseStreaming();
	LoaderState = ELB_DungeonRoomLoaderState::Failed;

	UE_LOG(LogLB, Warning, TEXT("[RoomLoader] %s: 방 로드 실패 %s (%s)"), *GetName(), *RequestedLevelName, Reason);
	OnRoomFailed.Broadcast(this);
}

void ALB_DungeonRoomLoader::ClearPendingCallbacks()
{
	if (bOnLevelShownBound && LevelStreaming)
	{
		LevelStreaming->OnLevelShown.RemoveDynamic(this, &ALB_DungeonRoomLoader::HandleLevelShown);
	}
	bOnLevelShownBound = false;

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(TimeoutHandle);
		TimerManager.ClearTimer(ReadyCheckHandle);
	}
	TimeoutHandle.Invalidate();
	ReadyCheckHandle.Invalidate();
}

void ALB_DungeonRoomLoader::ReleaseStreaming()
{
	if (LevelStreaming)
	{
		LevelStreaming->SetShouldBeVisible(false);
		LevelStreaming->SetShouldBeLoaded(false);
		LevelStreaming->SetIsRequestingUnloadAndRemoval(true);
	}
	LevelStreaming = nullptr;
}

void ALB_DungeonRoomLoader::UnloadRoomLocal()
{
	if (LoaderState != ELB_DungeonRoomLoaderState::Loading && LoaderState != ELB_DungeonRoomLoaderState::Ready)
	{
		// Idle/Failed(이미 정리됨)/Unloading(이미 요청됨)은 할 일 없음.
		return;
	}

	ClearPendingCallbacks();
	ReleaseStreaming();
	LoaderState = ELB_DungeonRoomLoaderState::Unloading;

	UE_LOG(LogLB, Log, TEXT("[RoomLoader] %s: 방 해제 요청 %s"), *GetName(), *RequestedLevelName);
}

ULevel* ALB_DungeonRoomLoader::GetLoadedRoomLevel() const
{
	if (LoaderState != ELB_DungeonRoomLoaderState::Ready || !LevelStreaming)
	{
		return nullptr;
	}
	return LevelStreaming->GetLoadedLevel();
}

void ALB_DungeonRoomLoader::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnloadRoomLocal();

	// Idle 등 상태와 무관하게 남은 delegate·timer를 확실히 정리한다.
	ClearPendingCallbacks();
	if (LoaderState == ELB_DungeonRoomLoaderState::Idle)
	{
		LoaderState = ELB_DungeonRoomLoaderState::Unloading;
	}

	Super::EndPlay(EndPlayReason);
}
