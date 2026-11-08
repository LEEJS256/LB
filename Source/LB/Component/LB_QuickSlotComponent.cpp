#include "Component/LB_QuickSlotComponent.h"
#include "Data/LB_DA_ItemInfo.h"
#include "Subsystem/LB_GISubsystem_Data.h"
#include "AbilitySystemComponent.h"
#include "LB_InventoryComponent.h"
#include "Net/UnrealNetwork.h"

ULB_QuickSlotComponent::ULB_QuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	SkillSlots.SetNum(SkillSlotCount);
	ItemSlots.SetNum(ItemSlotCount);
}

void ULB_QuickSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULB_QuickSlotComponent, SkillSlots);
	DOREPLIFETIME(ULB_QuickSlotComponent, ItemSlots);
}


// ------------------------------------------------------------------ 등록 / 해제

void ULB_QuickSlotComponent::RegisterSkill(int32 Index, FGameplayTag AbilityTag)
{
	FLB_QuickSlotEntry Entry;
	Entry.AbilityTag = AbilityTag;
	Server_SetSlot(ELB_QuickSlotType::Skill, Index, Entry);
}

void ULB_QuickSlotComponent::RegisterItem(int32 Index, FName ItemCode)
{
	FLB_QuickSlotEntry Entry;
	Entry.ItemCode = ItemCode;
	Server_SetSlot(ELB_QuickSlotType::Item, Index, Entry);
}

void ULB_QuickSlotComponent::ClearSlot(ELB_QuickSlotType Type, int32 Index)
{
	Server_SetSlot(Type, Index, FLB_QuickSlotEntry());
}

void ULB_QuickSlotComponent::Server_SetSlot_Implementation(ELB_QuickSlotType Type, int32 Index,
                                                           FLB_QuickSlotEntry Entry)
{
	// 스킬 칸은 보류
	if (Type != ELB_QuickSlotType::Item || !ItemSlots.IsValidIndex(Index))
	{
		return;
	}
 
	Entry.SkillCode = NAME_None;
 
	// 아이템 등록 검증: 소모품이고 실제로 보유 중인 것만 (장비 칸 토글 사용 방지)
	if (!Entry.ItemCode.IsNone())
	{
		const ULB_DA_ItemInfo* Info = GetItemInfo(Entry.ItemCode);
		if (!Info)
		{
			UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] 등록 실패: 마스터 테이블에 없는 아이템 (%s)"), *Entry.ItemCode.ToString());
			return;
		}
		if (!Info->ItemTypeTag.MatchesTag(TAG_Item_Consumable))
		{
			UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] 등록 실패: 소모품이 아님 (%s)"), *Entry.ItemCode.ToString());
			return;
		}
 
		const ULB_InventoryComponent* Inv = GetInventory();
		if (!Inv || Inv->GetItemCount(Entry.ItemCode) <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] 등록 실패: 인벤토리에 없음 (%s)"), *Entry.ItemCode.ToString());
			return;
		}
 
		// 같은 아이템이 다른 칸에 있으면 그 칸은 비움 (중복 등록 방지 = 사실상 이동)
		for (int32 i = 0; i < ItemSlots.Num(); ++i)
		{
			if (i != Index && ItemSlots[i].ItemCode == Entry.ItemCode)
			{
				ItemSlots[i] = FLB_QuickSlotEntry();
				OnSlotChanged.Broadcast(ELB_QuickSlotType::Item, i);
			}
		}
	}
 
	ItemSlots[Index] = Entry;
	OnSlotChanged.Broadcast(ELB_QuickSlotType::Item, Index); // 서버(리슨서버 호스트) 쪽 UI용, 클라는 OnRep에서
}

void ULB_QuickSlotComponent::SwapSlots(ELB_QuickSlotType Type, int32 IndexA, int32 IndexB)
{
	if (IndexA == IndexB) return;
	Server_SwapSlots(Type, IndexA, IndexB);
}

void ULB_QuickSlotComponent::Server_SwapSlots_Implementation(ELB_QuickSlotType Type, int32 IndexA, int32 IndexB)
{
	TArray<FLB_QuickSlotEntry>& Slots = (Type == ELB_QuickSlotType::Skill) ? SkillSlots : ItemSlots;
	if (IndexA == IndexB || !Slots.IsValidIndex(IndexA) || !Slots.IsValidIndex(IndexB))
	{
		return;
	}
 
	Slots.Swap(IndexA, IndexB);
	OnSlotChanged.Broadcast(Type, IndexA);
	OnSlotChanged.Broadcast(Type, IndexB);
}

// ------------------------------------------------------------------ 사용

void ULB_QuickSlotComponent::UseSkillSlot(int32 Index)
{
	// if (!SkillSlots.IsValidIndex(Index) || !SkillSlots[Index].AbilityTag.IsValid())
	// {
	// 	return;
	// }
	//
	// if (UAbilitySystemComponent* ASC = GetASC())
	// {
	// 	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(SkillSlots[Index].AbilityTag));
	// }
}

void ULB_QuickSlotComponent::UseItemSlot(int32 Index)
{
	if (!ItemSlots.IsValidIndex(Index) || ItemSlots[Index].ItemCode.IsNone())
	{
		return;
	}
	Server_UseItemSlot(Index);
}

void ULB_QuickSlotComponent::Server_UseItemSlot_Implementation(int32 Index)
{
	if (!ItemSlots.IsValidIndex(Index) || ItemSlots[Index].ItemCode.IsNone())
	{
		return;
	}
 
	ULB_InventoryComponent* Inv = GetInventory();
	if (!Inv) return;
 
	// 같은 아이템이 여러 스택이어도 첫 번째(수량 > 0) 스택부터 사용
	FGuid ItemInstanceId;
	if (!Inv->FindFirstEntryByCode(ItemSlots[Index].ItemCode, ItemInstanceId))
	{
		return; // 다 써서 수량 0
	}
 
	// 서버에서 호출하면 RPC가 로컬로 바로 실행됨 → UseItem(GE 적용 + 수량 차감)
	Inv->Server_UseItem(ItemInstanceId);
}

// ------------------------------------------------------------------ 조회 / 갱신

FLB_QuickSlotEntry ULB_QuickSlotComponent::GetEntry(ELB_QuickSlotType Type, int32 Index) const
{
	const TArray<FLB_QuickSlotEntry>& Slots = GetSlots(Type);
	return Slots.IsValidIndex(Index) ? Slots[Index] : FLB_QuickSlotEntry();
}

void ULB_QuickSlotComponent::RefreshItemSlots()
{
	for (int32 i = 0; i < ItemSlots.Num(); ++i)
	{
		OnSlotChanged.Broadcast(ELB_QuickSlotType::Item, i);
	}
}

void ULB_QuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ULB_InventoryComponent* Inv = GetInventory())
	{
		Inv->OnInventoryUpdated.AddUniqueDynamic(this, &ULB_QuickSlotComponent::HandleInventoryUpdated);
	}
}

void ULB_QuickSlotComponent::HandleInventoryUpdated()
{
	RefreshItemSlots();
}

void ULB_QuickSlotComponent::OnRep_SkillSlots()
{
	for (int32 i = 0; i < SkillSlots.Num(); ++i)
	{
		OnSlotChanged.Broadcast(ELB_QuickSlotType::Skill, i);
	}
}

void ULB_QuickSlotComponent::OnRep_ItemSlots()
{
	RefreshItemSlots();
}

ULB_InventoryComponent* ULB_QuickSlotComponent::GetInventory() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<ULB_InventoryComponent>() : nullptr;
}


const ULB_DA_ItemInfo* ULB_QuickSlotComponent::GetItemInfo(FName ItemCode) const
{
	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	return Data ? Data->GetItemInfo(ItemCode) : nullptr;
}
