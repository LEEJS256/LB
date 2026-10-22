// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LB_InventoryComponent.h"
#include "Component/LB_EquipComponent.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/LB_PlayerState.h"
#include "Data/LB_DA_Equipment.h"
#include "GameFramework/Pawn.h"

// Sets default values for this component's properties
ULB_InventoryComponent::ULB_InventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// PlayerState와 함께 컴포넌트의 데이터가 복제되도록 설정
	SetIsReplicatedByDefault(true);
}

void ULB_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULB_InventoryComponent, InventoryItems);
}

bool ULB_InventoryComponent::AddItem(ULB_DA_Equipment* ItemData, TSubclassOf<ALB_Equipment> EquipClass, int32 Count)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !ItemData || Count <= 0)
	{
		return false;
	}

	int32 AmountToAdd = Count;

	// 1. 기존 슬롯 중 중첩 가능한 슬롯 탐색 (MaxStackSize > 1 인 경우)
	if (ItemData->MaxStackSize > 1)
	{
		for (FLB_ItemEntry& Item : InventoryItems)
		{
			if (Item.ItemData == ItemData && Item.StackCount < ItemData->MaxStackSize)
			{
				const int32 SpaceLeft = ItemData->MaxStackSize - Item.StackCount;
				const int32 AddCount = FMath::Min(AmountToAdd, SpaceLeft);

				Item.StackCount += AddCount;
				AmountToAdd -= AddCount;

				if (AmountToAdd <= 0)
				{
					OnInventoryUpdated.Broadcast();
					return true;
				}
			}
		}
	}

	// 2. 남은 수량이 있거나 중첩 불가능(MaxStackSize == 1)인 경우 새 슬롯 생성
	while (AmountToAdd > 0)
	{
		FLB_ItemEntry NewItem;
		NewItem.ItemData = ItemData;
		NewItem.EquipmentClass = EquipClass;
		NewItem.StackCount = FMath::Min(AmountToAdd, ItemData->MaxStackSize);
		NewItem.bIsEquipped = false;

		InventoryItems.Add(NewItem);
		AmountToAdd -= NewItem.StackCount;
	}

	OnInventoryUpdated.Broadcast();
	return true;
}

bool ULB_InventoryComponent::RemoveItem(FGuid ItemInstanceId, int32 Count)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || Count <= 0)
	{
		return false;
	}

	int32 Index = InventoryItems.IndexOfByPredicate([&](const FLB_ItemEntry& Item) {
		return Item.ItemInstanceId == ItemInstanceId;
	});

	if (Index != INDEX_NONE)
	{
		FLB_ItemEntry& TargetItem = InventoryItems[Index];

		if (TargetItem.StackCount > Count)
		{
			TargetItem.StackCount -= Count;
		}
		else
		{
			// 수량이 0 이하가 되면 삭제하기 전에 장착부터 해제
			if (TargetItem.bIsEquipped)
			{
				UnequipItem(ItemInstanceId);
			}
			InventoryItems.RemoveAt(Index);
		}

		OnInventoryUpdated.Broadcast();
		return true;
	}

	return false;
}

bool ULB_InventoryComponent::EquipItem(FGuid ItemInstanceId)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	FLB_ItemEntry* TargetItem = InventoryItems.FindByPredicate([&](const FLB_ItemEntry& Item) {
		return Item.ItemInstanceId == ItemInstanceId;
	});

	if (!TargetItem || TargetItem->bIsEquipped || !TargetItem->ItemData)
	{
		return false;
	}

	ULB_EquipComponent* EquipComp = GetEquipComponent();
	if (!EquipComp)
	{
		return false;
	}

	// 기존 장착 상태 클리어
	for (FLB_ItemEntry& Item : InventoryItems)
	{
		if (Item.bIsEquipped)
		{
			Item.bIsEquipped = false;
		}
	}

	// DataAsset의 ItemTypeTag 및 지정된 액터 클래스를 활용해 월드에 장착
	EquipComp->EquipWeaponSet(TargetItem->ItemData->ItemTypeTag, TargetItem->EquipmentClass);

	TargetItem->bIsEquipped = true;
	OnInventoryUpdated.Broadcast();

	return true;
}

bool ULB_InventoryComponent::UnequipItem(FGuid ItemInstanceId)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	FLB_ItemEntry* TargetItem = InventoryItems.FindByPredicate([&](const FLB_ItemEntry& Item) {
		return Item.ItemInstanceId == ItemInstanceId;
	});

	if (!TargetItem || !TargetItem->bIsEquipped)
	{
		return false;
	}

	ULB_EquipComponent* EquipComp = GetEquipComponent();
	if (EquipComp)
	{
		EquipComp->UnequipWeaponSet();
	}

	TargetItem->bIsEquipped = false;
	OnInventoryUpdated.Broadcast();

	return true;
}

void ULB_InventoryComponent::OnRep_InventoryItems()
{
	OnInventoryUpdated.Broadcast();
}

ULB_EquipComponent* ULB_InventoryComponent::GetEquipComponent() const
{
	if (const ALB_PlayerState* PS = Cast<ALB_PlayerState>(GetOwner()))
	{
		if (const APawn* Pawn = PS->GetPawn())
		{
			return Pawn->FindComponentByClass<ULB_EquipComponent>();
		}
	}
	return nullptr;
}



