// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LB_InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Component/LB_EquipComponent.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/LB_PlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Subsystem/LB_GISubsystem_Data.h"
#include "Data/LB_DA_ItemInfo.h"
#include  "Data/LB_DT_WeaponInformation.h"
#include "Engine/World.h"
#include "Equipment/LB_Equipment.h"
#include "GameFramework/Pawn.h"
#include "Object/Item/LB_DropItem.h"
#include "Tests/ToolMenusTestUtilities.h"

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

int32 ULB_InventoryComponent::AddItem(FName ItemCode, int32 Count)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || Count <= 0) return Count;

	const ULB_DA_ItemInfo* Info = GetItemInfo(ItemCode);
	if (!Info) return Count;

	const int32 MaxStack = FMath::Max(1, Info->MaxStackSize);
	int32 Remain = Count;

	// 1. 기존 스택 채우기
	for (FLB_ItemEntry& Entry : InventoryItems)
	{
		if (Remain <= 0) break;
		if (Entry.ItemCode != ItemCode || Entry.StackCount >= MaxStack) continue;

		const int32 Add = FMath::Min(Remain, MaxStack - Entry.StackCount);
		Entry.StackCount += Add;
		Remain -= Add;
	}

	// 2. 새 슬롯
	while (Remain > 0 && InventoryItems.Num() < MaxSlots)
	{
		FLB_ItemEntry NewEntry;
		NewEntry.ItemInstanceId = FGuid::NewGuid();
		NewEntry.ItemCode = ItemCode;
		NewEntry.StackCount = FMath::Min(Remain, MaxStack);
		InventoryItems.Add(NewEntry);
		Remain -= NewEntry.StackCount;
	}

	if (Remain < Count) OnRep_InventoryItems();   // 서버(리슨서버 호스트)는 OnRep이 안 불려서 수동 호출
	return Remain;   // 못 넣은 개수
}


bool ULB_InventoryComponent::RemoveItem(FGuid ItemInstanceId, int32 Count)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || Count <= 0)
	{
		return false;
	}

	int32 Index = InventoryItems.IndexOfByPredicate([&](const FLB_ItemEntry& Item)
	{
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

		// OnInventoryUpdated.Broadcast();
		return true;
	}

	return false;
}

int32 ULB_InventoryComponent::GetItemCount(FName ItemCode) const
{
	int32 Total = 0;
	for (const FLB_ItemEntry& Entry : InventoryItems)
	{
		if (Entry.ItemCode == ItemCode)
		{
			Total += Entry.StackCount;
		}
	}
	return Total;
}

bool ULB_InventoryComponent::FindFirstEntryByCode(FName ItemCode, FGuid& OutId) const
{
	for (const FLB_ItemEntry& Entry : InventoryItems)
	{
		if (Entry.ItemCode == ItemCode && Entry.StackCount > 0)
		{
			OutId = Entry.ItemInstanceId;
			return true;
		}
	}
	return false;
}

void ULB_InventoryComponent::Server_UseItem_Implementation(FGuid ItemInstanceId)
{
	UseItem(ItemInstanceId);
}

void ULB_InventoryComponent::Server_DropItem_Implementation(FGuid ItemInstanceId, int32 Count)
{
	FLB_ItemEntry* Entry = InventoryItems.FindByPredicate(
		[&](const FLB_ItemEntry& E) { return E.ItemInstanceId == ItemInstanceId; });
	if (!Entry || !DropItemClass) return;

	const FName Code = Entry->ItemCode;
	Count = FMath::Clamp(Count, 1, Entry->StackCount);

	const ALB_PlayerState* PS = Cast<ALB_PlayerState>(GetOwner());
	const APawn* Pawn = PS ? PS->GetPawn() : nullptr;
	if (!Pawn) return;

	if (!RemoveItem(ItemInstanceId, Count)) return; // 이후 Entry 사용 금지

	const FTransform T(Pawn->GetActorRotation(),
	                   Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 100.f);

	ALB_DropItem* Drop = GetWorld()->SpawnActorDeferred<ALB_DropItem>(DropItemClass, T);
	if (Drop)
	{
		Drop->InitDrop(Code, Count);
		Drop->FinishSpawning(T);
	}
}

bool ULB_InventoryComponent::EquipItem(FGuid ItemInstanceId)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	FLB_ItemEntry* TargetItem = InventoryItems.FindByPredicate([&](const FLB_ItemEntry& Item)
	{
		return Item.ItemInstanceId == ItemInstanceId;
	});

	if (!TargetItem || TargetItem->bIsEquipped || TargetItem->ItemCode.IsNone()) return false;

	const ULB_DA_ItemInfo* Info = GetItemInfo(TargetItem->ItemCode);
	if (!Info || !Info->WeaponTag.IsValid() || Info->EquipClass.IsNull()) return false;

	TSubclassOf<ALB_Equipment> EquipClass = Info->EquipClass.LoadSynchronous();
	if (!EquipClass) return false;

	ULB_EquipComponent* EquipComp = GetEquipComponent();
	if (!EquipComp) return false;

	for (FLB_ItemEntry& Item : InventoryItems) Item.bIsEquipped = false;

	EquipComp->EquipWeaponSet(Info->WeaponTag, EquipClass);
	TargetItem->bIsEquipped = true;
	OnRep_InventoryItems();
	return true;
}

bool ULB_InventoryComponent::UnequipItem(FGuid ItemInstanceId)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	FLB_ItemEntry* TargetItem = InventoryItems.FindByPredicate([&](const FLB_ItemEntry& Item)
	{
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
	// OnInventoryUpdated.Broadcast();

	return true;
}

void ULB_InventoryComponent::OnRep_InventoryItems()
{
	// OnInventoryUpdated.Broadcast();
}

bool ULB_InventoryComponent::UseItem(FGuid ItemInstanceId)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return false;

	FLB_ItemEntry* Entry = InventoryItems.FindByPredicate(
		[&](const FLB_ItemEntry& E) { return E.ItemInstanceId == ItemInstanceId; });
	if (!Entry) return false;

	const ULB_DA_ItemInfo* Info = GetItemInfo(Entry->ItemCode);
	if (!Info) return false;

	if (Info->ItemTypeTag.MatchesTag(TAG_Item_Equip))
	{
		return Entry->bIsEquipped ? UnequipItem(ItemInstanceId) : EquipItem(ItemInstanceId);
	}

	if (Info->ItemTypeTag.MatchesTag(TAG_Item_Consumable))
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
		if (!ASC) return false;

		for (const TSubclassOf<UGameplayEffect>& GE : Info->Granted_GE)
		{
			if (!GE) continue;
			ASC->ApplyGameplayEffectToSelf(GE->GetDefaultObject<UGameplayEffect>(), 1.f, ASC->MakeEffectContext());
		}
		return RemoveItem(ItemInstanceId, 1);   // Entry는 이후 사용 금지
	}
	return false;
}

const class ULB_DA_ItemInfo* ULB_InventoryComponent::GetItemInfo(FName ItemCode) const
{
	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);

	return Data ? Data->GetItemInfo(ItemCode) : nullptr;
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
