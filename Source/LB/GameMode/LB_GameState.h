// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Data/Dungeon/LB_DungeonRoomTypes.h"
#include "LB_GameState.generated.h"

class ALB_DungeonRoomLoader;
class ALB_PlayerController;
class ULevel;

// UI용: 복제/갱신된 전환 상태 알림. 정책은 소유하지 않는다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLB_OnDungeonTransitionChanged, const FLB_DungeonTransitionSnapshot&, Snapshot);

// 서버 World의 Pending 방 표시 완료(RoomId, TransitionSerial).
DECLARE_MULTICAST_DELEGATE_TwoParams(FLB_OnServerPendingRoomReady, FName /*RoomId*/, int32 /*TransitionSerial*/);
// Client World의 Pending 방 표시 완료. 소유 PlayerController가 서버에 보고하는 데 사용(InstanceKey, RoomId, TransitionSerial).
DECLARE_MULTICAST_DELEGATE_ThreeParams(FLB_OnLocalPendingRoomReady, FName /*InstanceKey*/, FName /*RoomId*/, int32 /*TransitionSerial*/);
// 서버: 검증을 통과한 Client 준비 보고.
DECLARE_MULTICAST_DELEGATE_ThreeParams(FLB_OnClientPendingRoomReady, ALB_PlayerController* /*Reporter*/, FName /*RoomId*/, int32 /*TransitionSerial*/);
// 로컬 World의 Pending 방 로드 실패(RoomId, TransitionSerial). 재시도 정책은 소유하지 않는다.
DECLARE_MULTICAST_DELEGATE_TwoParams(FLB_OnLocalPendingRoomFailed, FName /*RoomId*/, int32 /*TransitionSerial*/);
// Client World: 안정 상태(Pending 없음·Waiting/CountingDown)의 Current 방 표시 완료. (InstanceKey, RoomId, TransitionSerial)당 한 번.
DECLARE_MULTICAST_DELEGATE_ThreeParams(FLB_OnLocalCurrentRoomReady, FName /*InstanceKey*/, FName /*RoomId*/, int32 /*TransitionSerial*/);
// 서버: 검증을 통과한 Client의 안정 상태 Current 방 준비 보고(늦은 합류).
DECLARE_MULTICAST_DELEGATE_ThreeParams(FLB_OnClientCurrentRoomReady, ALB_PlayerController* /*Reporter*/, FName /*RoomId*/, int32 /*TransitionSerial*/);

/**
 * 던전 Route Graph와 전환 Snapshot을 복제하고, 각 World에서 두 방 슬롯(A/B)의 로더 수명을 관리한다.
 * Exit 목적지 선택·과반수 판정·카운트다운 Timer·Fade·Pawn Teleport는 GameMode 몫이다.
 */
UCLASS()
class LB_API ALB_GameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ---- 서버 전용 ----

	// Route를 한 번만 게시한다(InstanceKey가 None일 때만, Validate 통과 필요).
	bool PublishDungeonRoute(const FLB_DungeonRouteState& NewRoute);

	// 전환 Snapshot을 갱신한다. Route 게시 후, RoomId가 Route에 있고 EntryId가 그 방 EntryIds에 선언돼 있어야 한다.
	bool UpdateDungeonTransition(const FLB_DungeonTransitionSnapshot& NewSnapshot);

	// Pending Room/Entry를 Current로 승격하고 ActiveExitId/PendingRoomId/PendingEntryId를 초기화한다. 이전 Current 로더는 해제된다.
	bool CompleteDungeonTransition();

	// Client 준비 보고 검증. 현재 InstanceKey/PendingRoomId/TransitionSerial과 일치하면 OnClientPendingRoomReady 발행.
	void ReceiveClientRoomReady(ALB_PlayerController* Reporter, FName InstanceKey, FName RoomId, int32 TransitionSerial);

	// 안정 상태 Current 방 준비 보고 검증. InstanceKey/CurrentRoomId/TransitionSerial 일치, PendingRoomId=None, 안정 Phase이면
	// OnClientCurrentRoomReady 발행.
	void ReceiveClientCurrentRoomReady(ALB_PlayerController* Reporter, FName InstanceKey, FName RoomId, int32 TransitionSerial);

	// ---- 조회 ----

	// 늦은 합류를 받는 안정 Phase(Waiting/CountingDown).
	static bool IsStableDungeonPhase(ELB_DungeonTransitionPhase Phase);

	// 로컬 World에서 안정 상태 Current 방이 Snapshot과 같은 방/세대로 표시 완료됐는지.
	bool IsLocalStableCurrentRoomReady() const;

	const FLB_DungeonRouteState& GetDungeonRoute() const { return Route; }
	const FLB_DungeonTransitionSnapshot& GetDungeonTransition() const { return TransitionSnapshot; }
	bool HasDungeonRoute() const { return !Route.InstanceKey.IsNone(); }
	const FLB_DungeonRoomDefinition* FindRoomDefinition(FName RoomId) const;

	// Pending 방이 표시된 경우 해당 ULevel, 아니면 nullptr.
	ULevel* GetPendingRoomLevel() const;
	// Current 방이 표시된 경우 해당 ULevel, 아니면 nullptr.
	ULevel* GetCurrentRoomLevel() const;

	ALB_DungeonRoomLoader* GetCurrentRoomLoader() const { return CurrentRoomLoader; }
	ALB_DungeonRoomLoader* GetPendingRoomLoader() const { return PendingRoomLoader; }

	// 세대 홀짝 슬롯 규칙: 짝수 A, 홀수 B.
	const FTransform& GetSlotTransformForSerial(int32 TransitionSerial) const;

	UFUNCTION(BlueprintPure, Category = "Dungeon")
	FLB_DungeonTransitionSnapshot GetDungeonTransitionSnapshot() const { return TransitionSnapshot; }

	// ---- Delegates ----

	UPROPERTY(BlueprintAssignable, Category = "Dungeon")
	FLB_OnDungeonTransitionChanged OnDungeonTransitionChanged;

	FLB_OnServerPendingRoomReady OnServerPendingRoomReady;
	FLB_OnLocalPendingRoomReady OnLocalPendingRoomReady;
	FLB_OnClientPendingRoomReady OnClientPendingRoomReady;
	FLB_OnLocalPendingRoomFailed OnLocalPendingRoomFailed;
	FLB_OnLocalCurrentRoomReady OnLocalCurrentRoomReady;
	FLB_OnClientCurrentRoomReady OnClientCurrentRoomReady;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_Route();

	UFUNCTION()
	void OnRep_TransitionSnapshot();

	UPROPERTY(EditDefaultsOnly, Category = "Dungeon")
	FTransform RoomSlotATransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, Category = "Dungeon")
	FTransform RoomSlotBTransform = FTransform(FVector(100000.f, 0.f, 0.f));

private:
	// BeginPlay/OnRep/서버 갱신 공통 로컬 적용 경로.
	void ApplyDungeonStateLocal();

	// 비복제 로더를 스폰만 한다. 호출자가 멤버 대입·delegate 바인딩 후 LoadRoomLocal을 호출한다.
	ALB_DungeonRoomLoader* SpawnRoomLoader();
	void ReleaseLoader(TObjectPtr<ALB_DungeonRoomLoader>& Loader);

	UFUNCTION()
	void HandlePendingRoomReady(ALB_DungeonRoomLoader* Loader);

	UFUNCTION()
	void HandlePendingRoomFailed(ALB_DungeonRoomLoader* Loader);

	UFUNCTION()
	void HandleCurrentRoomFailed(ALB_DungeonRoomLoader* Loader);

	UFUNCTION()
	void HandleCurrentRoomReady(ALB_DungeonRoomLoader* Loader);

	// Client: 안정 상태 Current 방 표시 완료를 (Key, Room, Serial)당 한 번 알린다.
	void TryNotifyLocalCurrentRoomReady();

	UPROPERTY(ReplicatedUsing = OnRep_Route)
	FLB_DungeonRouteState Route;

	UPROPERTY(ReplicatedUsing = OnRep_TransitionSnapshot)
	FLB_DungeonTransitionSnapshot TransitionSnapshot;

	UPROPERTY(Transient)
	TObjectPtr<ALB_DungeonRoomLoader> CurrentRoomLoader;

	UPROPERTY(Transient)
	TObjectPtr<ALB_DungeonRoomLoader> PendingRoomLoader;

	// 각 로더를 만든 세대(Level 이름·슬롯 판정용).
	int32 CurrentRoomSerial = INDEX_NONE;
	int32 PendingRoomSerial = INDEX_NONE;

	bool bPendingReadyNotified = false;

	// 마지막으로 알린 안정 Current 방(중복 알림 방지).
	FName NotifiedCurrentInstanceKey = NAME_None;
	FName NotifiedCurrentRoomId = NAME_None;
	int32 NotifiedCurrentSerial = INDEX_NONE;
};
