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
