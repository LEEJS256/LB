// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "LB_DungeonRoomTypes.generated.h"

/**
 * 순서형 Route를 구성하는 방 하나의 정의.
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
};

/**
 * 런타임에 확정된 순서형 방 Route.
 * Layout Data Asset 참조 없이 독립 복사본으로 유지된다.
 */
USTRUCT(BlueprintType)
struct LB_API FLB_DungeonRouteState
{
	GENERATED_BODY()

	// 작은 고정 던전 프로토타입 경계.
	static constexpr int32 MaxRooms = 32;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	FName InstanceKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon")
	TArray<FLB_DungeonRoomDefinition> Rooms;

	// InstanceKey와 Rooms 전체를 검사한다. 실패 시 OutError에 원인을 남긴다.
	bool Validate(FString& OutError) const;

	// Layout과 Route가 공유하는 방 목록 규격 검사.
	static bool ValidateRooms(const TArray<FLB_DungeonRoomDefinition>& InRooms, FString& OutError);

	// 영문/숫자/underscore로만 이루어진 None이 아닌 이름인지 검사.
	static bool IsValidIdentifier(const FName& InName);
};
