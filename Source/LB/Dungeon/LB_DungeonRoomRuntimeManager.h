// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Dungeon/LB_DungeonRoomRuntimeTypes.h"
#include "LB_DungeonRoomRuntimeManager.generated.h"

class ULevel;
class ALB_MonsterBase;
class ALB_DestructibleProp;
struct FLB_PropBreakState;

// 서버 전용. 활성 방의 살아 있는 몬스터 수가 바뀌었을 때(활성화 성공 포함) 발행. (RoomId, LivingMonsterCount)
DECLARE_MULTICAST_DELEGATE_TwoParams(FLB_OnRoomCombatStateChanged, FName /*RoomId*/, int32 /*LivingMonsterCount*/);

/**
 * 서버 전용 방 전투 상태 관리자. 비복제, Tick 비활성.
 *
 * 계약:
 * - 현재 InstanceKey 세션 동안 RoomId별 소비 ID(사망 몬스터/파괴 Prop)를 메모리에 유지한다. Deactivate 뒤에도 유지된다.
 * - ActivateRoom은 표시된 방 Streaming ULevel의 Monster/Prop Marker를 전부 검증한 뒤 소비되지 않은 Marker만 Spawn한다.
 *   Spawn·delegate 연결이 하나라도 실패하면 이번 시도의 Actor/구독을 전부 롤백하고 false. 누적 상태는 변경하지 않는다.
 * - Runtime Actor는 OverrideLevel로 방 ULevel에 생성되어 그 수명을 따른다.
 * - Client 복제·SaveGame·체력/AI 위치·DropItem 보존은 하지 않는다.
 */
UCLASS(NotBlueprintable)
class LB_API ALB_DungeonRoomRuntimeManager : public AActor
{
	GENERATED_BODY()

public:
	ALB_DungeonRoomRuntimeManager();

	// 세션 InstanceKey 설정. 이전과 다른 키면 활성 방을 비활성화하고 누적 상태를 비운다.
	void SetSessionInstanceKey(FName InInstanceKey);
	FName GetSessionInstanceKey() const { return SessionInstanceKey; }

	/**
	 * 서버 전용. RoomId 방을 LoadedLevel(표시된 방 Streaming Level)로 활성화한다.
	 * - 이미 다른 RoomId가 활성이면 거부(false). 같은 RoomId+Level이면 이미 활성 상태로 true.
	 * - Marker 0개도 유효(빈 Runtime).
	 */
	bool ActivateRoom(FName RoomId, ULevel* LoadedLevel);

	// 서버 전용. 구독 해제 후 남은 관리 Actor 제거. 생존/미파괴 대상은 소비 처리하지 않는다.
	void DeactivateRoom();

	bool IsRoomActive() const { return !ActiveRoomId.IsNone(); }
	FName GetActiveRoomId() const { return ActiveRoomId; }

	bool HasLivingMonsters() const { return LivingMonsterIds.Num() > 0; }
	int32 GetLivingMonsterCount() const { return LivingMonsterIds.Num(); }

	// 활성 방의 누적 상태. 비활성이거나 기록이 없으면 nullptr.
	const FLB_DungeonRoomRuntimeState* GetRoomState() const;

	// 임의 RoomId의 누적 상태. 없으면 nullptr.
	const FLB_DungeonRoomRuntimeState* FindRoomState(FName RoomId) const;

	FLB_OnRoomCombatStateChanged& OnRoomCombatStateChanged() { return OnRoomCombatStateChangedDelegate; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FManagedSpawn
	{
		FName SpawnId = NAME_None;
		TWeakObjectPtr<AActor> Actor;
		FDelegateHandle Handle;
		bool bMonster = false;
	};

	bool IsServerWorld() const;

	// 관리 Actor 구독 해제 및(옵션) 제거, 활성 방 참조 정리.
	void ReleaseManagedSpawns(bool bDestroyActors);

	static void UnbindManagedSpawn(FManagedSpawn& Spawn);

	void HandleMonsterDefeated(ALB_MonsterBase* Monster, FName SpawnId);
	void HandlePropBroken(ALB_DestructibleProp* Prop, const FLB_PropBreakState& BreakState, FName SpawnId);

	void BroadcastCombatStateChanged();

	FName SessionInstanceKey = NAME_None;

	// RoomId -> 누적 소비 상태 (서버 메모리 전용)
	TMap<FName, FLB_DungeonRoomRuntimeState> RoomStates;

	FName ActiveRoomId = NAME_None;
	TWeakObjectPtr<ULevel> ActiveLevel;

	// ActivateRoom Spawn 단계 진행 중. 이 동안의 결과 신호는 기록하지 않고 커밋 시점에 Actor 상태로 반영한다.
	bool bActivationInProgress = false;

	TArray<FManagedSpawn> ManagedSpawns;
	TSet<FName> LivingMonsterIds;

	FLB_OnRoomCombatStateChanged OnRoomCombatStateChangedDelegate;
};
