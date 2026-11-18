// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LB_LootTableDataAsset.generated.h"

/** Loot Table의 추첨 후보 한 항목. 선택 시 [MinCount, MaxCount] 범위에서 Stack 수량을 정한다. */
USTRUCT(BlueprintType)
struct LB_API FLB_LootEntry
{
	GENERATED_BODY()

	/** MasterItem DT의 Row 이름. NAME_None이면 무효 Entry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName ItemCode = NAME_None;

	/** 가중치. 비유한 또는 0 이하이면 무효 Entry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	float Weight = 1.f;

	/** 선택 시 Stack 최소 수량(1 이상). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 MinCount = 1;

	/** 선택 시 Stack 최대 수량(MinCount 이상). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 MaxCount = 1;
};

/** 추첨 1회의 결과. 동일 ItemCode가 여러 결과로 나올 수 있으며 합산하지 않는다. */
USTRUCT(BlueprintType)
struct LB_API FLB_LootResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName ItemCode = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 Count = 0;
};

/**
 * 항아리 등 파괴 오브젝트용 Loot Table.
 * DropChance로 한 번 판정 후, [MinRollCount, MaxRollCount]회 가중치 복원 추출한다.
 */
UCLASS(BlueprintType)
class LB_API ULB_LootTableDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxAllowedRollCount = 16;

	/** 항아리당 한 번 판정하는 드롭 확률. 유한값만 허용하며 [0,1]로 Clamp한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	float DropChance = 0.5f;

	/** 추첨 횟수 최소(0..16). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "16"))
	int32 MinRollCount = 1;

	/** 추첨 횟수 최대(MinRollCount..16). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "16"))
	int32 MaxRollCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FLB_LootEntry> Entries;

	/**
	 * 주입된 RandomStream만 사용해 Loot를 추첨한다. OutResults는 먼저 비운다.
	 * @return 결과가 1개 이상이면 true. 확률 실패·0회·무효 설정이면 false.
	 */
	bool RollLoot(FRandomStream& Random, TArray<FLB_LootResult>& OutResults) const;
};
