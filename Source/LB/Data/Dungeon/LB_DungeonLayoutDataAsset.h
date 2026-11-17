// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Dungeon/LB_DungeonRoomTypes.h"
#include "LB_DungeonLayoutDataAsset.generated.h"

/**
 * 작성용 고정 던전 Layout. 시작 방과 Room/Exit Graph를 보유한다.
 * 런타임에는 BuildRoute로 만든 독립 Route 복사본을 사용한다.
 */
UCLASS(BlueprintType)
class LB_API ULB_DungeonLayoutDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 파티가 처음 진입하는 방. Rooms에 존재해야 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dungeon")
	FName StartRoomId = NAME_None;

	// StartRoom 안의 첫 도착 Entry. StartRoom.EntryIds에 선언돼 있어야 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dungeon")
	FName StartEntryId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dungeon")
	TArray<FLB_DungeonRoomDefinition> Rooms;

	// StartRoomId와 Room/Exit Graph를 런타임 Route와 같은 규격으로 검사한다.
	bool ValidateLayout(FString& OutError) const;

	// OutRoute를 기본값으로 초기화한 뒤, 성공 시 StartRoomId/StartEntryId/Rooms(Entry·Exit 포함)를 값 복사한 독립 Route를 채운다.
	// Data Asset은 수정하지 않는다. 서버 전용 호출 보장은 호출 측(GameMode) 책임.
	bool BuildRoute(FName InstanceKey, FLB_DungeonRouteState& OutRoute, FString& OutError) const;
};
