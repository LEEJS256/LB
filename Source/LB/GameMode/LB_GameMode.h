// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Data/Dungeon/LB_DungeonRoomTypes.h"
#include "LB_GameMode.generated.h"

class ALB_GameState;
class ALB_PlayerController;
class ULB_DungeonLayoutDataAsset;

/**
 * 서버 던전 진행 권한자. 고정 Layout으로 Route를 한 번 게시하고,
 * 방 전환마다 서버 World와 참가 Client의 준비 보고를 모으는 Barrier를 관리한다.
 */
UCLASS()
class LB_API ALB_GameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Dungeon")
	TObjectPtr<ULB_DungeonLayoutDataAsset> StartupDungeonLayout;

	// 방 준비 Barrier 제한 시간(초). 0 이하이면 Timeout을 사용하지 않는다.
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon", meta = (ClampMin = "0.0"))
	float RoomReadyTimeoutSeconds = 30.f;

	// 서버 World와 모든 참가 Controller가 같은 Pending 방/세대를 보고했을 때 한 번 호출된다.
	void HandleRoomBarrierReady();

	// 참가자를 캡처하고 Barrier를 연 뒤 Snapshot을 게시한다. 게시 실패 시 Barrier를 닫는다.
	bool BeginRoomTransition(const FLB_DungeonTransitionSnapshot& NewSnapshot);

private:
	void BindGameStateDelegates();
	void UnbindGameStateDelegates();

	void HandleServerPendingRoomReady(FName RoomId, int32 TransitionSerial);
	void HandleClientPendingRoomReady(ALB_PlayerController* Reporter, FName RoomId, int32 TransitionSerial);
	void HandleLocalPendingRoomFailed(FName RoomId, int32 TransitionSerial);
	void HandleRoomReadyTimeout();

	static bool IsBarrierParticipantCandidate(const APlayerController* PC);
	bool IsBarrierReport(FName RoomId, int32 TransitionSerial) const;
	void TryCompleteRoomBarrier();
	void ClearRoomBarrier();
	void FailRoomTransition(const TCHAR* Reason);

	UPROPERTY(Transient)
	TObjectPtr<ALB_GameState> DungeonGameState;

	FDelegateHandle ServerReadyHandle;
	FDelegateHandle ClientReadyHandle;
	FDelegateHandle LocalFailedHandle;

	FTimerHandle RoomReadyTimeoutHandle;

	// 현재 Barrier 대상(전환 시작 시 잠김).
	bool bBarrierActive = false;
	FName BarrierInstanceKey = NAME_None;
	FName BarrierRoomId = NAME_None;
	int32 BarrierSerial = INDEX_NONE;
	bool bServerRoomReady = false;
	TSet<TWeakObjectPtr<APlayerController>> BarrierParticipants;
	TSet<TWeakObjectPtr<APlayerController>> ReadyParticipants;

	// Snapshot 게시 중 동기 완료 재진입 방지.
	bool bPublishingTransition = false;
	bool bFailureDuringPublish = false;
};
