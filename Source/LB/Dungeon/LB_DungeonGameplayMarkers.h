// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "LB_DungeonGameplayMarkers.generated.h"

class USceneComponent;
class ULevel;
class ALB_MonsterBase;
class ALB_DestructibleProp;

/**
 * 방 Streaming Level에 배치하는 게임플레이 Spawn Marker 베이스.
 *
 * 계약:
 * - 방(RoomId) 단위로 Monster/Prop Marker는 종류와 무관하게 SpawnId 하나의 고유 집합을 공유한다.
 *   (ValidateUniqueSpawnIds로 검사)
 * - SpawnId는 None이 아니고 영문/숫자/underscore로만 구성되어야 한다.
 * - Marker는 데이터 작성 전용이다. BeginPlay Spawn, 복제, Actor 상태, RoomId, Loot, Exit 제어를
 *   수행하지 않는다. 실제 Spawn은 서버 Runtime Manager가 Marker Transform과 소유 ULevel을 조회해 수행한다.
 */
UCLASS(Abstract, Blueprintable)
class LB_API ALB_DungeonGameplaySpawnMarker : public AActor
{
	GENERATED_BODY()

public:
	ALB_DungeonGameplaySpawnMarker();

	FName GetSpawnId() const { return SpawnId; }

	// SpawnId가 None이 아니고 영문/숫자/underscore로만 구성되었는지 검사.
	bool HasValidSpawnId() const;
	static bool IsValidSpawnId(const FName& InSpawnId);

	// Spawn 기준 Transform (Root 컴포넌트 기준, Scale 제외).
	FTransform GetSpawnTransform() const;

	// Runtime Actor를 생성할 Marker 소유 ULevel (방 Streaming Level).
	ULevel* GetSpawnLevel() const;

	// Marker가 참조하는 Spawn Actor Class가 설정되어 있는지.
	virtual bool HasSpawnClass() const { return false; }

	/**
	 * 같은 방의 Marker 집합에서 SpawnId 형식과 종류 무관 고유성을 검사한다.
	 * null Marker는 무시한다. 실패 시 OutError에 원인 기록.
	 */
	static bool ValidateUniqueSpawnIds(const TArray<const ALB_DungeonGameplaySpawnMarker*>& Markers, FString& OutError);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon|Spawn")
	TObjectPtr<USceneComponent> Root;

	// 방 내 안정 식별자. Monster/Prop Marker 전체에서 고유해야 한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Dungeon|Spawn")
	FName SpawnId;
};

/** 몬스터 Spawn Marker. */
UCLASS(Blueprintable)
class LB_API ALB_DungeonMonsterSpawnMarker : public ALB_DungeonGameplaySpawnMarker
{
	GENERATED_BODY()

public:
	ALB_DungeonMonsterSpawnMarker();

	TSubclassOf<ALB_MonsterBase> GetMonsterClass() const { return MonsterClass; }
	virtual bool HasSpawnClass() const override;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Dungeon|Spawn")
	TSubclassOf<ALB_MonsterBase> MonsterClass;
};

/** 파괴 오브젝트(Prop) Spawn Marker. */
UCLASS(Blueprintable)
class LB_API ALB_DungeonPropSpawnMarker : public ALB_DungeonGameplaySpawnMarker
{
	GENERATED_BODY()

public:
	ALB_DungeonPropSpawnMarker();

	TSubclassOf<ALB_DestructibleProp> GetPropClass() const { return PropClass; }
	virtual bool HasSpawnClass() const override;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Dungeon|Spawn")
	TSubclassOf<ALB_DestructibleProp> PropClass;
};
