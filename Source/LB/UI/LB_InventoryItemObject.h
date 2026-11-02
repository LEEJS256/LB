// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Utility/LB_ItemEntry.h"
#include "LB_InventoryItemObject.generated.h"

/**
 * TileView 한 칸의 데이터. ListView는 UObject만 받아서 FLB_ItemEntry를 감싸는 용도.
 * - 슬롯 번호 = InventoryItems 배열 인덱스
 * - 배열 범위를 넘는 칸(남는 칸)은 빈 칸 객체 (ItemCode == None)
 * 아이콘/이름 같은 마스터 데이터는 안 들고, 엔트리 위젯이 ItemCode로 조회한다.
 */
UCLASS(BlueprintType)
class LB_API ULB_InventoryItemObject : public UObject
{
	GENERATED_BODY()

public:
	/** 실제 아이템이 있는 칸 */
	void InitItem(int32 InSlotIndex, const FLB_ItemEntry& InEntry);

	/** 빈 칸 */
	void InitEmpty(int32 InSlotIndex);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsEmpty() const { return Entry.ItemCode.IsNone(); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const FLB_ItemEntry& GetEntry() const { return Entry; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FName GetItemCode() const { return Entry.ItemCode; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetStackCount() const { return Entry.StackCount; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsEquipped() const { return Entry.bIsEquipped; }

	/** Server_UseItem / Server_DropItem에 넘길 ID. 빈 칸이면 무효 GUID */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FGuid GetItemInstanceId() const { return Entry.ItemInstanceId; }

private:
	UPROPERTY()
	FLB_ItemEntry Entry;

	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;
};