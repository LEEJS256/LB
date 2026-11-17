#include "LB_EquipPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "PaperSprite.h"
#include "TimerManager.h"
#include "Component/LB_InventoryComponent.h"
#include "Component/LB_QuickSlotComponent.h" // LBSlotGroup
#include "Data/LB_DA_EquipInfo.h"
#include "PlayerState/LB_PlayerState.h"
#include "Subsystem/LB_GISubsystem_Data.h"
#include "UI/SlateWidget/LB_EquipSlot.h"
#include "Utility/LB_ItemEntry.h"

// ============================================================
// 생명주기
// ============================================================

void ULB_EquipPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	CollectSlots();
}

void ULB_EquipPanel::NativeConstruct()
{
	Super::NativeConstruct();
	TryBindInventory();
}

void ULB_EquipPanel::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryHandle);
	}
	if (Inventory.IsValid())
	{
		Inventory->OnInventoryUpdated.RemoveDynamic(this, &ULB_EquipPanel::HandleInventoryUpdated);
	}
	Super::NativeDestruct();
}

// ============================================================
// 슬롯 수집 / 바인딩
// ============================================================

void ULB_EquipPanel::CollectSlots()
{
	SlotByKey.Reset();
	SlotByIndex.Reset();

	if (!WidgetTree) return;

	// 디자이너에 배치된 ULB_EquipSlot 전부 수집 (이름/위치 상관없음)
	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		ULB_EquipSlot* EquipSlot = Cast<ULB_EquipSlot>(Widget);
		if (!EquipSlot) return;

		const FLB_EquipSlotKey Key = EquipSlot->GetSlotKey();
		const int32 Index = EquipSlot->GetSlotIndex();

		if (!Key.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[EquipPanel] %s: SlotKey 미지정"), *EquipSlot->GetName());
			return;
		}
		if (SlotByKey.Contains(Key))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EquipPanel] %s: SlotKey 중복 (%s[%d])"),
				*EquipSlot->GetName(), *Key.SlotTag.ToString(), Key.Index);
			return;
		}
		if (Index == INDEX_NONE || SlotByIndex.Contains(Index))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EquipPanel] %s: SlotIndex 미지정 또는 중복 (%d)"),
				*EquipSlot->GetName(), Index);
			return;
		}

		SlotByKey.Add(Key, EquipSlot);
		SlotByIndex.Add(Index, EquipSlot);

		EquipSlot->OnEquipDropped.AddUniqueDynamic(this, &ULB_EquipPanel::HandleEquipDropped);
		EquipSlot->OnUnequip.AddUniqueDynamic(this, &ULB_EquipPanel::HandleUnequip);
		EquipSlot->CanAcceptDrop.BindUObject(this, &ULB_EquipPanel::HandleCanAccept);
	});

	UE_LOG(LogTemp, Log, TEXT("[EquipPanel] 장비 슬롯 %d칸 수집"), SlotByKey.Num());
}

ULB_EquipSlot* ULB_EquipPanel::FindSlot(const FLB_EquipSlotKey& Key) const
{
	const TObjectPtr<ULB_EquipSlot>* Found = SlotByKey.Find(Key);
	return Found ? Found->Get() : nullptr;
}

bool ULB_EquipPanel::GetSlotKeyByIndex(int32 SlotIndex, FLB_EquipSlotKey& OutKey) const
{
	const TObjectPtr<ULB_EquipSlot>* Found = SlotByIndex.Find(SlotIndex);
	if (!Found || !*Found) return false;

	OutKey = (*Found)->GetSlotKey();
	return true;
}

void ULB_EquipPanel::RefreshFromEquipped(const TMap<FLB_EquipSlotKey, FName>& Equipped)
{
	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);

	for (const TPair<FLB_EquipSlotKey, TObjectPtr<ULB_EquipSlot>>& Pair : SlotByKey)
	{
		ULB_EquipSlot* EquipSlot = Pair.Value;
		if (!EquipSlot) continue;

		const FName* ItemCode = Equipped.Find(Pair.Key);
		const ULB_DA_ItemInfo* Info = (ItemCode && Data) ? Data->GetItemInfo(*ItemCode) : nullptr;

		if (Info)
		{
			// TODO: 아이콘이 많아지면 비동기 로드로 교체
			EquipSlot->SetItem(Info->Icon.LoadSynchronous(), *ItemCode);
		}
		else
		{
			EquipSlot->ClearItem();
		}
	}
}

// ============================================================
// 슬롯 이벤트
// ============================================================

void ULB_EquipPanel::HandleEquipDropped(FLB_EquipSlotKey DstKey, FName SrcGroup, int32 SrcIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[EquipPanel] Drop %s[%d] -> %s[%d]"),
		*SrcGroup.ToString(), SrcIndex, *DstKey.SlotTag.ToString(), DstKey.Index);

	// 1) 인벤토리 → 장비칸: 장착
	if (SrcGroup == LBSlotGroup::Inventory())
	{
		RequestEquip(SrcIndex, DstKey);
		return;
	}

	// 2) 장비칸 → 장비칸: 같은 종류끼리만 교환 (악세 0~3 사이)
	if (SrcGroup == EquipGroup())
	{
		FLB_EquipSlotKey SrcKey;
		if (GetSlotKeyByIndex(SrcIndex, SrcKey) && SrcKey.SlotTag == DstKey.SlotTag)
		{
			RequestSwap(SrcKey, DstKey);
		}
		return;
	}

	// 퀵슬롯 → 장비칸 등은 무시
}

void ULB_EquipPanel::HandleUnequip(FLB_EquipSlotKey Key)
{
	RequestUnequip(Key);
}

bool ULB_EquipPanel::HandleCanAccept(const FLB_EquipSlotKey& DstKey, FName SrcGroup, int32 SrcIndex) const
{
	// 인벤토리에서 온 아이템: 장비이고, 들어갈 슬롯 태그가 이 칸과 같아야 함
	if (SrcGroup == LBSlotGroup::Inventory())
	{
		const ULB_DA_EquipInfo* EquipInfo = ResolveEquipInfo(ResolveInventoryItemCode(SrcIndex));
		return EquipInfo && EquipInfo->EquipSlotTag.MatchesTagExact(DstKey.SlotTag);
	}

	// 장비칸끼리: 같은 종류만 (악세끼리)
	if (SrcGroup == EquipGroup())
	{
		FLB_EquipSlotKey SrcKey;
		return GetSlotKeyByIndex(SrcIndex, SrcKey) && SrcKey.SlotTag == DstKey.SlotTag;
	}

	return false;
}

// ============================================================
// 인벤토리 연동
// ============================================================

void ULB_EquipPanel::TryBindInventory()
{
	// 멀티에선 PlayerState 복제가 늦을 수 있어서 준비될 때까지 재시도
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const ALB_PlayerState* PS = PC->GetPlayerState<ALB_PlayerState>())
		{
			if (ULB_InventoryComponent* Inv = PS->Get_InventoryComponent())
			{
				Inventory = Inv;
				Inv->OnInventoryUpdated.AddUniqueDynamic(this, &ULB_EquipPanel::HandleInventoryUpdated);
				HandleInventoryUpdated();
				return;
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BindRetryHandle, this, &ULB_EquipPanel::TryBindInventory, 0.2f, false);
	}
}

void ULB_EquipPanel::HandleInventoryUpdated()
{
	if (!Inventory.IsValid()) return;

	// TODO: 장착 데이터가 생기면 여기서 컴포넌트의 장착 맵을 받아서 갱신
	// RefreshFromEquipped(Inventory->GetEquippedItems());
}

FName ULB_EquipPanel::ResolveInventoryItemCode(int32 InventoryIndex) const
{
	if (!Inventory.IsValid()) return NAME_None;

	const TArray<FLB_ItemEntry>& Items = Inventory->GetInventoryItems();
	return Items.IsValidIndex(InventoryIndex) ? Items[InventoryIndex].ItemCode : NAME_None;
}

const ULB_DA_EquipInfo* ULB_EquipPanel::ResolveEquipInfo(FName ItemCode) const
{
	if (ItemCode.IsNone()) return nullptr;

	const ULB_GISubsystem_Data* Data = ULB_GISubsystem_Data::Get_Subsystem_Data(this);
	return Data ? Cast<ULB_DA_EquipInfo>(Data->GetItemInfo(ItemCode)) : nullptr;
}

// ============================================================
// 컴포넌트 요청 — 장착 데이터 작업 때 서버 RPC로 연결
// ============================================================

void ULB_EquipPanel::RequestEquip(int32 InventoryIndex, const FLB_EquipSlotKey& DstKey)
{
	if (!Inventory.IsValid()) return;

	// TODO: Inventory->Server_EquipToSlot(InventoryIndex, DstKey);
	UE_LOG(LogTemp, Log, TEXT("[EquipPanel] RequestEquip Inv[%d] -> %s[%d]"),
		InventoryIndex, *DstKey.SlotTag.ToString(), DstKey.Index);
}

void ULB_EquipPanel::RequestUnequip(const FLB_EquipSlotKey& Key)
{
	if (!Inventory.IsValid()) return;

	// TODO: Inventory->Server_UnequipSlot(Key);
	UE_LOG(LogTemp, Log, TEXT("[EquipPanel] RequestUnequip %s[%d]"), *Key.SlotTag.ToString(), Key.Index);
}

void ULB_EquipPanel::RequestSwap(const FLB_EquipSlotKey& A, const FLB_EquipSlotKey& B)
{
	if (!Inventory.IsValid()) return;

	// TODO: Inventory->Server_SwapEquipSlots(A, B);
	UE_LOG(LogTemp, Log, TEXT("[EquipPanel] RequestSwap %s[%d] <-> %s[%d]"),
		*A.SlotTag.ToString(), A.Index, *B.SlotTag.ToString(), B.Index);
}