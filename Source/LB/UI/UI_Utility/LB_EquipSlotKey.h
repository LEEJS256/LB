#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LB_EquipSlotKey.generated.h"

/**
 * 장비 슬롯 키.
 * - 방어구/무기: SlotTag만으로 구분 (Index = 0)
 * - 악세사리   : SlotTag = Equip.Slot.Accessory, Index = 0~3
 */
USTRUCT(BlueprintType)
struct LB_API FLB_EquipSlotKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Equip"))
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 Index = 0;

	FLB_EquipSlotKey() = default;
	FLB_EquipSlotKey(const FGameplayTag& InTag, int32 InIndex = 0) : SlotTag(InTag), Index(InIndex) {}

	bool IsValid() const { return SlotTag.IsValid() && Index >= 0; }

	bool operator==(const FLB_EquipSlotKey& Other) const
	{
		return SlotTag == Other.SlotTag && Index == Other.Index;
	}
	bool operator!=(const FLB_EquipSlotKey& Other) const { return !(*this == Other); }

	friend uint32 GetTypeHash(const FLB_EquipSlotKey& Key)
	{
		return HashCombine(GetTypeHash(Key.SlotTag), ::GetTypeHash(Key.Index));
	}
};