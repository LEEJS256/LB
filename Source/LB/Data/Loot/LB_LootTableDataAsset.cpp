// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/Loot/LB_LootTableDataAsset.h"

namespace LBLootTable
{
	static bool IsValidEntry(const FLB_LootEntry& Entry)
	{
		return !Entry.ItemCode.IsNone()
			&& FMath::IsFinite(Entry.Weight)
			&& Entry.Weight > 0.f
			&& Entry.MinCount >= 1
			&& Entry.MaxCount >= Entry.MinCount;
	}

	/** [Min, Max] 양끝 포함 정수. int32 전 범위에서 차이/덧셈 overflow가 없도록 int64로 계산한다. */
	static int32 RandRangeInclusiveSafe(FRandomStream& Random, int32 Min, int32 Max)
	{
		if (Max <= Min)
		{
			return Min;
		}

		const uint64 Span = static_cast<uint64>(static_cast<int64>(Max) - static_cast<int64>(Min)) + 1ull; // 1..2^32
		const uint64 Offset = static_cast<uint64>(Random.GetUnsignedInt()) % Span;
		return static_cast<int32>(static_cast<int64>(Min) + static_cast<int64>(Offset));
	}
}

bool ULB_LootTableDataAsset::RollLoot(FRandomStream& Random, TArray<FLB_LootResult>& OutResults) const
{
	OutResults.Reset();

	// 추첨 횟수 설정 검증: 범위 밖/역전은 전체 실패.
	if (MinRollCount < 0 || MaxRollCount < 0
		|| MinRollCount > MaxAllowedRollCount || MaxRollCount > MaxAllowedRollCount
		|| MaxRollCount < MinRollCount)
	{
		return false;
	}

	if (!FMath::IsFinite(DropChance))
	{
		return false;
	}

	// 유효 Entry와 가중치 합(double).
	TArray<int32, TInlineAllocator<16>> ValidIndices;
	double TotalWeight = 0.0;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FLB_LootEntry& Entry = Entries[Index];
		if (LBLootTable::IsValidEntry(Entry))
		{
			ValidIndices.Add(Index);
			TotalWeight += static_cast<double>(Entry.Weight);
		}
	}

	if (ValidIndices.Num() == 0 || !FMath::IsFinite(TotalWeight) || TotalWeight <= 0.0)
	{
		return false;
	}

	// 항아리당 한 번의 확률 판정. 0은 반드시 실패, 1은 반드시 통과.
	const float Chance = FMath::Clamp(DropChance, 0.f, 1.f);
	if (Chance <= 0.f)
	{
		return false;
	}
	if (Chance < 1.f && Random.GetFraction() >= Chance)
	{
		return false;
	}

	const int32 RollCount = LBLootTable::RandRangeInclusiveSafe(Random, MinRollCount, MaxRollCount);
	if (RollCount <= 0)
	{
		return false;
	}

	OutResults.Reserve(RollCount);
	for (int32 Roll = 0; Roll < RollCount; ++Roll)
	{
		// 복원 추출: 매회 전체 유효 Entry에서 가중치 비례 선택.
		const double Pick = static_cast<double>(Random.GetFraction()) * TotalWeight;

		int32 SelectedIndex = ValidIndices.Last(); // 경계값은 마지막 유효 Entry로 마무리
		double Accumulated = 0.0;
		for (const int32 Index : ValidIndices)
		{
			Accumulated += static_cast<double>(Entries[Index].Weight);
			if (Pick < Accumulated)
			{
				SelectedIndex = Index;
				break;
			}
		}

		const FLB_LootEntry& Selected = Entries[SelectedIndex];
		FLB_LootResult& Result = OutResults.AddDefaulted_GetRef();
		Result.ItemCode = Selected.ItemCode;
		Result.Count = LBLootTable::RandRangeInclusiveSafe(Random, Selected.MinCount, Selected.MaxCount);
	}

	return OutResults.Num() > 0;
}
