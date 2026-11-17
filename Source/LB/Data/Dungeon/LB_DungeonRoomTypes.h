// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "LB_DungeonRoomTypes.generated.h"

/**
 * 방에서 다른 방으로 이어지는 단방향 출구 하나.
 */
USTRUCT(BlueprintType)
struct LB_API FLB_DungeonExitLink
{
	GENERATED_BODY()

	// 출구 식별자. 영문/숫자/underscore만 허용, 같은 방 안에서 None/중복 금지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FName ExitId = NAME_None;

	// 목적지 방 RoomId. 존재하는 다른 방이어야 한다(Self Link 금지).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FName TargetRoomId = NAME_None;
};

/**
 * Room/Exit Graph를 구성하는 방 하나의 정의.
 * 작성용 Layout과 런타임 Route State가 같은 규격을 공유한다.
 */
USTRUCT(BlueprintType)
struct LB_API FLB_DungeonRoomDefinition
{
	GENERATED_BODY()

	// 방 식별자. 영문/숫자/underscore만 허용, None/중복 금지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FName RoomId = NAME_None;

	// 스트리밍할 방 맵. 같은 맵을 다른 RoomId로 반복 사용할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	TSoftObjectPtr<UWorld> Level;

	// 이 방의 출구 목록(0..MaxExitsPerRoom). 비어 있으면 종료 방.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	TArray<FLB_DungeonExitLink> Exits;
};

/**
 * 런타임에 확정된 방 Route(StartRoom + Room/Exit DAG).
 * Layout Data Asset 참조 없이 독립 복사본으로 유지된다.
 */
USTRUCT(BlueprintType)
struct LB_API FLB_DungeonRouteState
{
	GENERATED_BODY()

	// 작은 고정 던전 프로토타입 경계.
	static constexpr int32 MaxRooms = 32;
	static constexpr int32 MaxExitsPerRoom = 4;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	FName InstanceKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	FName StartRoomId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	TArray<FLB_DungeonRoomDefinition> Rooms;

	// InstanceKey, StartRoomId, Room/Exit Graph 전체를 검사한다. 실패 시 OutError에 원인을 남긴다.
	bool Validate(FString& OutError) const;

	// Layout과 Route가 공유하는 방 목록 + Exit Graph 규격 검사.
	// 방 1..MaxRooms, 방별 Exit 0..MaxExitsPerRoom, ID 형식/고유성, 목적지 존재,
	// Self Link/Cycle 금지, StartRoomId에서 모든 방 도달 가능. 분기 후 합류하는 DAG는 허용.
	static bool ValidateGraph(FName InStartRoomId, const TArray<FLB_DungeonRoomDefinition>& InRooms, FString& OutError);

	// 영문/숫자/underscore로만 이루어진 None이 아닌 이름인지 검사.
	static bool IsValidIdentifier(const FName& InName);
};

/**
 * 방 전환 진행 단계. 서버 GameMode가 결정하고 GameState가 복제한다.
 */
UENUM(BlueprintType)
enum class ELB_DungeonTransitionPhase : uint8
{
	Waiting,
	CountingDown,
	LoadingNextRoom,
	FadingOut,
	FadingIn,
	Failed
};

/**
 * 현재 방 전환 상태의 복제 Snapshot.
 * TransitionSerial은 PendingRoomId가 유효하면 Pending 방의 세대, 아니면 현재 방의 세대다.
 * 세대 홀짝으로 방 슬롯(짝수 A, 홀수 B)을 고르므로 새 Pending은 이전 세대 + 1로 게시해야 한다.
 */
USTRUCT(BlueprintType)
struct LB_API FLB_DungeonTransitionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	FName CurrentRoomId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	FName PendingRoomId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	FName ActiveExitId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	int32 TransitionSerial = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	ELB_DungeonTransitionPhase Phase = ELB_DungeonTransitionPhase::Waiting;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	int32 TotalPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	int32 NearbyPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	int32 RequiredPlayers = 0;

	// 카운트다운 종료 서버 시간(GetServerWorldTimeSeconds 기준). 0이면 카운트다운 없음.
	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	double CountdownEndServerTime = 0.0;
};
