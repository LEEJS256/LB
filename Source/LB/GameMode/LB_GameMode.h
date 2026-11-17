// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Data/Dungeon/LB_DungeonRoomTypes.h"
#include "LB_GameMode.generated.h"

class ALB_GameState;
class ALB_PlayerController;
class ALB_DungeonExit;
class ALB_DungeonRoomEntry;
class ULB_DungeonLayoutDataAsset;
class ULevel;
class APawn;

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

	/**
	 * 서버: 호출 시점에 접속한 파티로 던전을 시작한다(Route 게시 + 첫 방 전환).
	 * Authority·미시작·Route 미게시·참가자 1명 이상일 때만 성공하며, 한 번 성공(또는 Route 게시) 후에는 다시 실행되지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	bool StartDungeon();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Dungeon")
	TObjectPtr<ULB_DungeonLayoutDataAsset> StartupDungeonLayout;

	// 방 준비 Barrier 제한 시간(초). 0 이하이면 Timeout을 사용하지 않는다.
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon", meta = (ClampMin = "0.0"))
	float RoomReadyTimeoutSeconds = 30.f;

	// 과반수 도달 후 전환까지의 카운트다운(초). 양수여야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float ExitCountdownSeconds = 5.f;

	// 목적지 방 준비 후 Teleport 전 Fade Out 시간(초).
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.5f;

	// Teleport·승격 후 Fade In 시간(초).
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon", meta = (ClampMin = "0.0"))
	float FadeInDuration = 0.5f;

	// Entry 기준 로컬 Y축으로 파티 Pawn을 배치하는 간격(cm).
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon", meta = (ClampMin = "0.0"))
	float PartyEntrySpacing = 150.f;

	// 서버 World와 모든 참가 Controller가 같은 Pending 방/세대를 보고했을 때 한 번 호출된다.
	void HandleRoomBarrierReady();

	// 참가자를 캡처하고 Barrier를 연 뒤 Snapshot을 게시한다. 게시 실패 시 Barrier를 닫는다.
	bool BeginRoomTransition(const FLB_DungeonTransitionSnapshot& NewSnapshot);

private:
	// ---- 파티 ----

	// 전환 대상: 유효 PlayerState·Pawn을 가진 비관전자 Controller(Listen Server 로컬 포함).
	static bool IsPartyCandidate(const APlayerController* PC);
	void GatherParty(TArray<APlayerController*>& OutParty) const;
	void CaptureTransitionParty();

	void WatchPartyController(APlayerController* PC);
	void UnwatchPartyController(APlayerController* PC);
	void WatchPartyPawn(APawn* Pawn);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandlePartyPawnDestroyed(AActor* DestroyedActor);

	// 참가자 변화 후 다음 Tick에 모든 Exit를 다시 평가한다(중복 요청은 하나로 합친다).
	void RequestExitReevaluation();
	void HandleDeferredExitReevaluation();

	// ---- Exit 평가 ----

	void BindCurrentRoomExits();
	void UnbindCurrentRoomExits();
	void HandleExitPresenceChanged(ALB_DungeonExit* Exit);

	bool CanEvaluateExits() const;
	void EvaluateAllExits();
	void EvaluateExit(ALB_DungeonExit* Exit);
	int32 CountNearbyParty(const ALB_DungeonExit* Exit, const TArray<APlayerController*>& Party) const;
	FName FindTargetRoomForExit(FName ExitId) const;
	ALB_DungeonExit* FindBoundExit(FName ExitId) const;
	float GetCountdownDuration() const;

	void StartExitCountdown(FName ExitId, int32 Nearby, int32 Required, int32 Total);
	void UpdateExitCountdownCounts(int32 Nearby, int32 Required, int32 Total);
	void CancelExitCountdown();
	void HandleExitCountdownFinished(FName ExitId, int32 RoomSerial);

	// 선택 Exit/목적지 전환을 한 번만 잠근다. RoomSerial은 평가 시점 Current 세대.
	void LockRoomTransition(FName ExitId, FName TargetRoomId, int32 RoomSerial);

	// ---- 진입 연출 ----

	// 표시된 방 Level의 Entry가 하나이고 Exit Marker 집합이 Graph와 정확히 일치하는지 검사한다.
	static bool ValidateRoomMarkers(ULevel* Level, const FLB_DungeonRoomDefinition& Room, ALB_DungeonRoomEntry*& OutEntry, FString& OutError);

	void BroadcastDungeonFade(bool bFadeOut, float Duration);
	void ScheduleTransitionStep(float Delay, void (ALB_GameMode::*Step)(int32), int32 Serial);
	void HandleFadeOutFinished(int32 Serial);
	void HandleFadeInFinished(int32 Serial);
	void TeleportPartyToEntry(const ALB_DungeonRoomEntry* Entry);
	static bool TeleportControllerToEntry(APlayerController* PC, const ALB_DungeonRoomEntry* Entry, float OffsetY);
	void ClearTransitionTimers();
	void BindGameStateDelegates();
	void UnbindGameStateDelegates();

	void HandleServerPendingRoomReady(FName RoomId, int32 TransitionSerial);
	void HandleClientPendingRoomReady(ALB_PlayerController* Reporter, FName RoomId, int32 TransitionSerial);
	void HandleLocalPendingRoomFailed(FName RoomId, int32 TransitionSerial);
	void HandleRoomReadyTimeout();

	// ---- 안정 상태 늦은 합류 ----

	// GameState가 검증한 안정 상태 Current 방 준비 보고. Reporter 한 명만 현재 Entry로 옮긴다.
	void HandleClientCurrentRoomReady(ALB_PlayerController* Reporter, FName RoomId, int32 TransitionSerial);
	// 보관한 안정 Entry가 현재 방/세대와 맞으면 사용하고, 아니면 서버 Current Level에서 다시 검증한다.
	ALB_DungeonRoomEntry* ResolveCurrentRoomEntry();

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
	FDelegateHandle CurrentReadyHandle;

	// StartDungeon 성공(Route 게시) 후 true. 중복 시작을 막는다.
	bool bDungeonStarted = false;

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

	// ---- Exit/전환 상태 ----

	// 안정된 Current 방에서 구독 중인 Exit와 그 방/세대.
	TArray<TWeakObjectPtr<ALB_DungeonExit>> BoundExits;
	FName BoundExitsRoomId = NAME_None;
	int32 BoundExitsSerial = INDEX_NONE;

	FTimerHandle ExitCountdownHandle;
	FTimerHandle TransitionStepHandle;
	FTimerHandle ReevaluateHandle;

	// 목적지 잠금부터 Fade In 완료까지 true. Exit 이벤트·Timer 완료를 무시한다.
	bool bTransitionLocked = false;

	// 잠금 시 캡처한 이동 대상 파티(출구 밖 포함).
	TArray<TWeakObjectPtr<APlayerController>> TransitionParty;

	// Barrier 완료 시 검증한 Pending 방 Entry.
	TWeakObjectPtr<ALB_DungeonRoomEntry> PendingEntry;

	// 승격 완료된 Current 방 Entry와 그 방/세대. 이전 방 해제로 파괴되면 Weak 포인터가 무효가 된다.
	TWeakObjectPtr<ALB_DungeonRoomEntry> CurrentEntry;
	FName CurrentEntryRoomId = NAME_None;
	int32 CurrentEntrySerial = INDEX_NONE;

	// 현재 방 Entry에 이미 배치된 Controller(파티 Teleport 성공 + 늦은 합류 완료). 중복 합류 보고를 무시한다.
	TSet<TWeakObjectPtr<APlayerController>> CurrentRoomPlaced;
};
