// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LB_DungeonRoomRuntimeTypes.generated.h"

/**
 * 서버 메모리 전용 방 전투 결과 상태.
 *
 * 계약:
 * - RoomId는 Route가 생성한 방 노드 고유 ID다. 같은 Level을 쓰는 다른 RoomId와 공유하지 않는다.
 * - 소비된(사망한 몬스터/파괴된 Prop) SpawnId만 기록한다. 체력·AI 위치·DropItem 등 중간 상태는 저장하지 않는다.
 * - Client 복제나 디스크 SaveGame 대상이 아니다.
 */
USTRUCT(BlueprintType)
struct FLB_DungeonRoomRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon|Runtime")
	FName RoomId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon|Runtime")
	TSet<FName> DefeatedMonsterIds;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon|Runtime")
	TSet<FName> BrokenPropIds;

	bool IsMonsterDefeated(FName SpawnId) const { return DefeatedMonsterIds.Contains(SpawnId); }
	bool IsPropBroken(FName SpawnId) const { return BrokenPropIds.Contains(SpawnId); }

	// 종류와 무관하게 이미 소비된 SpawnId인지.
	bool IsSpawnConsumed(FName SpawnId) const { return IsMonsterDefeated(SpawnId) || IsPropBroken(SpawnId); }

	// 멱등 기록. 새로 추가되었을 때만 true.
	bool RecordMonsterDefeated(FName SpawnId)
	{
		if (SpawnId.IsNone())
		{
			return false;
		}
		bool bAlreadyInSet = false;
		DefeatedMonsterIds.Add(SpawnId, &bAlreadyInSet);
		return !bAlreadyInSet;
	}

	// 멱등 기록. 새로 추가되었을 때만 true.
	bool RecordPropBroken(FName SpawnId)
	{
		if (SpawnId.IsNone())
		{
			return false;
		}
		bool bAlreadyInSet = false;
		BrokenPropIds.Add(SpawnId, &bAlreadyInSet);
		return !bAlreadyInSet;
	}
};
