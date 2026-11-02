// Fill out your copyright notice in the Description page of Project Settings.


#include "LB_InventoryItemObject.h"

void ULB_InventoryItemObject::InitItem(int32 InSlotIndex, const FLB_ItemEntry& InEntry)
{
	SlotIndex = InSlotIndex;
	Entry = InEntry;
}

void ULB_InventoryItemObject::InitEmpty(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	// FLB_ItemEntry 기본 생성자는 NewGuid()를 만들고 StackCount가 1이라 빈 칸용으로 되돌린다
	Entry = FLB_ItemEntry();
	Entry.ItemInstanceId.Invalidate();
	Entry.ItemCode = NAME_None;
	Entry.StackCount = 0;
	Entry.bIsEquipped = false;
}