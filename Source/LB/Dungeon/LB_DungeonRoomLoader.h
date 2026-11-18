// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Dungeon/LB_DungeonRoomTypes.h"
#include "LB_DungeonRoomLoader.generated.h"

class ULevel;
class ULevelStreamingDynamic;
class ALB_DungeonRoomLoader;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLB_OnDungeonRoomReady, ALB_DungeonRoomLoader*, Loader);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLB_OnDungeonRoomFailed, ALB_DungeonRoomLoader*, Loader);

UENUM(BlueprintType)
enum class ELB_DungeonRoomLoaderState : uint8
{
	// 요청 전.
	Idle,
	// 요청 수락, 표시 대기 중.
	Loading,
	// 방이 로드·표시 완료됨.
	Ready,
	// 초기 호출 실패 또는 Timeout. 재사용하지 않는다.
	Failed,
	// 해제 요청됨. 재사용하지 않는다.
	Unloading
};

/**
 * 한 방(Room) 하나만 로컬 World에 스트리밍하는 로더.
 * 복제하지 않으며 서버/Client가 같은 입력으로 같은 Level 이름을 만든다.
 * Route 선택, 슬롯 관리, Fade, Pawn 이동은 담당하지 않는다.
 */
UCLASS()
class LB_API ALB_DungeonRoomLoader : public AActor
{
	GENERATED_BODY()

public:
	ALB_DungeonRoomLoader();

	/**
	 * Room 하나를 SlotTransform 위치에 비동기 로드 요청한다(로더 Actor Transform은 합성하지 않음).
	 * 반환값은 요청 수락 여부이며 표시 완료는 OnRoomReady로 알린다.
	 * 동일 요청 재호출은 기존 수락 결과를 반환하고, 다른 요청·실패/해제 후 재사용은 거부한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon|RoomLoader")
	bool LoadRoomLocal(FName InstanceKey, int32 TransitionSerial, const FLB_DungeonRoomDefinition& Room, const FTransform& SlotTransform);

	// 로드 중이거나 표시된 방의 해제를 요청한다. 이후 이 로더는 재사용하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "Dungeon|RoomLoader")
	void UnloadRoomLocal();

	// Ready 상태일 때 표시된 방 ULevel, 그 외에는 nullptr.
	UFUNCTION(BlueprintPure, Category = "Dungeon|RoomLoader")
	ULevel* GetLoadedRoomLevel() const;

	UFUNCTION(BlueprintPure, Category = "Dungeon|RoomLoader")
	ELB_DungeonRoomLoaderState GetLoaderState() const { return LoaderState; }

	UFUNCTION(BlueprintPure, Category = "Dungeon|RoomLoader")
	bool IsRoomReady() const { return LoaderState == ELB_DungeonRoomLoaderState::Ready; }

	UFUNCTION(BlueprintPure, Category = "Dungeon|RoomLoader")
	FName GetRequestedRoomId() const { return RequestedRoomId; }

	UFUNCTION(BlueprintPure, Category = "Dungeon|RoomLoader")
	FString GetRequestedLevelName() const { return RequestedLevelName; }

	// 서버/Client 공통 Level 이름 Override 규칙: LB_<InstanceKey>_<TransitionSerial>_<RoomId>.
	static FString MakeRoomLevelName(FName InstanceKey, int32 TransitionSerial, FName RoomId);

	// 표시 완료 시 한 번 발행.
	UPROPERTY(BlueprintAssignable, Category = "Dungeon|RoomLoader")
	FLB_OnDungeonRoomReady OnRoomReady;

	// 초기 호출 실패 또는 Timeout 시 한 번 발행.
	UPROPERTY(BlueprintAssignable, Category = "Dungeon|RoomLoader")
	FLB_OnDungeonRoomFailed OnRoomFailed;

	// 요청 수락 후 표시 완료까지 허용 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|RoomLoader", meta = (ClampMin = "0.1"))
	float LoadTimeoutSeconds = 30.f;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleLevelShown();

	void CheckReadyState();
	void HandleLoadTimeout();

	void CompleteReady();
	void FailRequest(const TCHAR* Reason);

	// delegate·timer 해제.
	void ClearPendingCallbacks();
	// Streaming 객체에 비표시·언로드·제거를 요청하고 참조를 끊는다.
	void ReleaseStreaming();

	bool IsSameRequest(FName InstanceKey, int32 TransitionSerial, const FLB_DungeonRoomDefinition& Room, const FTransform& SlotTransform) const;

	UPROPERTY(Transient)
	TObjectPtr<ULevelStreamingDynamic> LevelStreaming;

	ELB_DungeonRoomLoaderState LoaderState = ELB_DungeonRoomLoaderState::Idle;

	// 요청 식별 정보(동일 요청 판정용).
	FName RequestedInstanceKey = NAME_None;
	int32 RequestedTransitionSerial = 0;
	FName RequestedRoomId = NAME_None;
	FSoftObjectPath RequestedLevelPath;
	FTransform RequestedSlotTransform = FTransform::Identity;
	FString RequestedLevelName;

	// 수락 여부. 초기 호출 실패면 false.
	bool bRequestAccepted = false;
	bool bOnLevelShownBound = false;

	FTimerHandle TimeoutHandle;
	FTimerHandle ReadyCheckHandle;
};
