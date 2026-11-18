// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LB_InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Component/LB_EquipComponent.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/LB_PlayerState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Data/LB_DA_EquipInfo.h"
#include "Subsystem/LB_GISubsystem_Data.h"
#include "Data/LB_DA_ItemInfo.h"
#include "Engine/World.h"
#include "Equipment/LB_Equipment.h"
#include "GameFramework/Pawn.h"
#include "Object/Item/LB_DropItem.h"

// ============================================================
// 생명주기 / 복제
// ============================================================

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
	DOREPLIFETIME(ULB_InventoryComponent, EquippedItems);
}

// ============================================================
// 조회
// ============================================================

int32 ULB_InventoryComponent::Get_CurrentSlot() const
{
	return CurrentSlots;
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

void ULB_InventoryComponent::GetEquippedMap(TMap<FLB_EquipSlotKey, FName>& Out) const
{
	Out.Reset();
	for (const FLB_EquippedItem& E : EquippedItems)
	{
		Out.Add(E.SlotKey, E.Item.ItemCode);
	}
}

// ============================================================
// 인벤토리 조작 (서버)
// ============================================================

int32 ULB_InventoryComponent::AddItem(FName ItemCode, int32 Count)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || Count <= 0)
		return Count;

	const ULB_DA_ItemInfo* Info = GetItemInfo(ItemCode);
	if (!Info)
		return Count;

	// 인벤토리 배열 크기를 현재 해금된 슬롯 수(CurrentSlots)에 맞게 항상 고정 유지
	if (InventoryItems.Num() < CurrentSlots)
	{
		InventoryItems.SetNum(CurrentSlots);
	}

	const int32 MaxStack = FMath::Max(1, Info->MaxStackSize);
	int32 Remain = Count;

	// 1. 기존에 같은 아이템이 있는 스택 채우기
	for (FLB_ItemEntry& Entry : InventoryItems)
	{
		if (Remain <= 0) break;
		if (Entry.ItemCode != ItemCode || Entry.StackCount >= MaxStack) continue;

		const int32 Add = FMath::Min(Remain, MaxStack - Entry.StackCount);
		Entry.StackCount += Add;
		Remain -= Add;
	}

	// 2. 비어 있는 슬롯(ItemCode.IsNone())에 새로 채우기
	for (FLB_ItemEntry& Entry : InventoryItems)
	{
		if (Remain <= 0) break;
		if (!Entry.ItemCode.IsNone()) continue; // 빈 칸이 아니면 패스

		Entry.ItemInstanceId = FGuid::NewGuid();
		Entry.ItemCode = ItemCode;
		Entry.StackCount = FMath::Min(Remain, MaxStack);
		Remain -= Entry.StackCount;
	}

	// UE_LOG(LogTemp, Warning, TEXT("[AddItem] 결과 Remain=%d Slots=%d/%d MaxStack=%d"),
	//        Remain, InventoryItems.Num(), CurrentSlots, MaxStack);

	if (Remain < Count)
	{
		OwnerActor->ForceNetUpdate();
		OnRep_InventoryItems(); // 서버(리슨서버 호스트)는 OnRep이 안 불려서 수동 호출
		OnInventoryUpdated.Broadcast();
	}
	return Remain; // 못 넣은 개수
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
			TargetItem = FLB_ItemEntry();
			// InventoryItems.RemoveAt(Index);
		}

		OnInventoryUpdated.Broadcast();
		return true;
	}

	return false;
}

void ULB_InventoryComponent::ExpandInventory(int32 AddCount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	CurrentSlots = FMath::Clamp(CurrentSlots + AddCount, 0, MaxSlots);

	if (InventoryItems.Num() < CurrentSlots)
	{
		InventoryItems.SetNum(CurrentSlots);
	}
	// UI 갱신을 위해 브로드캐스트
	OnInventoryUpdated.Broadcast();
}

// ============================================================
// 인벤토리 UI 요청 (RPC)
// ============================================================

void ULB_InventoryComponent::Server_UseItem_Implementation(FGuid ItemInstanceId)
{
	UseItem(ItemInstanceId);
}

void ULB_InventoryComponent::Server_MoveItem_Implementation(int32 SrcIndex, int32 DstIndex)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
		return;

	if (SrcIndex < 0 || SrcIndex >= CurrentSlots || DstIndex < 0 || DstIndex >= CurrentSlots)
		return;
	if (SrcIndex == DstIndex)
		return;

	// 만약 InventoryItems 배열의 크기가 현재 가진 아이템 개수까지만 잡혀있다면, 
	// MaxSlots 크기까지 빈 슬롯(기본 생성된 빈 FLB_ItemEntry)으로 미리 채워져 있어야 합니다.
	// 배열 크기가 MaxSlots보다 작다면 빈 칸들을 채워주는 안전장치:
	while (InventoryItems.Num() < CurrentSlots)
	{
		FLB_ItemEntry EmptyEntry;
		// 빈 칸을 나타내는 식별자나 초기값 설정 (ItemCode가 None이면 빈 칸으로 간주)
		InventoryItems.Add(EmptyEntry);
	}

	// 고정 슬롯 방식이므로 두 인덱스의 데이터를 안전하게 교환(Swap)
	InventoryItems.Swap(SrcIndex, DstIndex);

	OwnerActor->ForceNetUpdate();
	OnRep_InventoryItems();
	OnInventoryUpdated.Broadcast();
}

void ULB_InventoryComponent::Server_SortItems_Implementation()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
		return;

	// 퀵슬롯은 ItemCode로 아이템을 찾으니까 배열 순서가 바뀌어도 영향 없음
	InventoryItems.StableSort([](const FLB_ItemEntry& A, const FLB_ItemEntry& B)
	{
		const bool bAEmpty = A.ItemCode.IsNone();
		const bool bBEmpty = B.ItemCode.IsNone();
		if (bAEmpty != bBEmpty)
			return !bAEmpty;            // 빈 칸은 뒤로
		if (bAEmpty)
			return false;               // 빈 칸끼리는 순서 유지

		if (A.StackCount != B.StackCount)
			return A.StackCount > B.StackCount;   // 많은 수가 위로

		return A.ItemCode.LexicalLess(B.ItemCode); // 같은 수량이면 코드 순 (같은 아이템끼리 붙게)
	});

	OwnerActor->ForceNetUpdate();
	OnRep_InventoryItems(); // 서버(리슨서버 호스트)는 OnRep이 안 불려서 수동 호출 (안에서 OnInventoryUpdated 브로드캐스트)
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

// ============================================================
// 장비 (서버)
// ============================================================

bool ULB_InventoryComponent::EquipItem(FGuid ItemInstanceId)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
		return false;

	const int32 InvIndex = InventoryItems.IndexOfByPredicate(
		[&](const FLB_ItemEntry& E) { return E.ItemInstanceId == ItemInstanceId; });
	if (InvIndex == INDEX_NONE)
		return false;

	const ULB_DA_EquipInfo* Info = Cast<ULB_DA_EquipInfo>(GetItemInfo(InventoryItems[InvIndex].ItemCode));
	FLB_EquipSlotKey Key;
	if (!Info || !FindFreeEquipKey(GetEquipSlotTag(Info), Key))
		return false;

	return EquipFromInventory(InvIndex, Key);
}

bool ULB_InventoryComponent::UnequipItem(FGuid ItemInstanceId)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
		return false;

	const FLB_EquippedItem* Found = EquippedItems.FindByPredicate(
		[&](const FLB_EquippedItem& E) { return E.Item.ItemInstanceId == ItemInstanceId; });

	return Found ? UnequipToInventory(Found->SlotKey) : false;
}

bool ULB_InventoryComponent::EquipFromInventory(int32 InventoryIndex, const FLB_EquipSlotKey& DstKey)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !IsValidEquipKey(DstKey))
		return false;

	if (!InventoryItems.IsValidIndex(InventoryIndex) || InventoryItems[InventoryIndex].ItemCode.IsNone())
		return false;

	// UI의 CanAccept와 같은 규칙을 서버에서 한 번 더 검사 (클라 요청은 믿지 않는다)
	const ULB_DA_EquipInfo* Info = Cast<ULB_DA_EquipInfo>(GetItemInfo(InventoryItems[InventoryIndex].ItemCode));
	if (!Info || !GetEquipSlotTag(Info).MatchesTagExact(DstKey.SlotTag))
		return false;

	if (InventoryItems[InventoryIndex].StackCount > 1)
	{
		// 장비는 MaxStackSize 1 전제. 겹쳐 있으면 어느 쪽을 장착할지 애매해서 거부
		UE_LOG(LogTemp, Warning, TEXT("[Inventory] 스택된 장비(%s x%d)는 장착할 수 없음"),
			*InventoryItems[InventoryIndex].ItemCode.ToString(), InventoryItems[InventoryIndex].StackCount);
		return false;
	}

	FLB_ItemEntry NewItem = InventoryItems[InventoryIndex];
	NewItem.bIsEquipped = true;

	const int32 ExistingIdx = FindEquippedIndex(DstKey);
	if (ExistingIdx != INDEX_NONE)
	{
		// 칸이 차 있으면 교체: 기존 장비는 방금 비는 인벤토리 칸으로 돌려보낸다
		FLB_ItemEntry OldItem = EquippedItems[ExistingIdx].Item;
		RemoveEquipEffects(OldItem.ItemInstanceId);
		RemoveWeaponActor(OldItem);

		OldItem.bIsEquipped = false;
		InventoryItems[InventoryIndex] = OldItem;
		EquippedItems[ExistingIdx].Item = NewItem;
	}
	else
	{
		InventoryItems[InventoryIndex] = FLB_ItemEntry(); // 빈 칸

		FLB_EquippedItem& Added = EquippedItems.AddDefaulted_GetRef();
		Added.SlotKey = DstKey;
		Added.Item = NewItem;
	}

	ApplyEquipEffects(NewItem);
	ApplyWeaponActor(NewItem);

	NotifyEquipChanged();
	return true;
}

bool ULB_InventoryComponent::UnequipToInventory(const FLB_EquipSlotKey& Key, int32 PreferredIndex)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
		return false;

	const int32 EquipIdx = FindEquippedIndex(Key);
	if (EquipIdx == INDEX_NONE)
		return false;

	if (InventoryItems.Num() < CurrentSlots)
	{
		InventoryItems.SetNum(CurrentSlots);
	}

	int32 FreeIdx = INDEX_NONE;
	if (PreferredIndex >= 0 && PreferredIndex < CurrentSlots)
	{
		const FLB_ItemEntry& Dst = InventoryItems[PreferredIndex];
		if (Dst.ItemCode.IsNone())
		{
			FreeIdx = PreferredIndex;
		}
		else
		{
			// 같은 부위 장비 위에 놓으면 그 장비를 장착 = 맞바꾸기 (기존 장비가 그 칸으로 감)
			const ULB_DA_EquipInfo* DstInfo = Cast<ULB_DA_EquipInfo>(GetItemInfo(Dst.ItemCode));
			if (DstInfo && GetEquipSlotTag(DstInfo).MatchesTagExact(Key.SlotTag))
			{
				return EquipFromInventory(PreferredIndex, Key);
			}
		}
	}

	if (FreeIdx == INDEX_NONE)
	{
		FreeIdx = InventoryItems.IndexOfByPredicate(
			[](const FLB_ItemEntry& E) { return E.ItemCode.IsNone(); });
	}
	if (FreeIdx == INDEX_NONE || FreeIdx >= CurrentSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inventory] 인벤토리가 가득 차서 해제할 수 없음"));
		return false;
	}

	FLB_ItemEntry Item = EquippedItems[EquipIdx].Item;
	RemoveEquipEffects(Item.ItemInstanceId);
	RemoveWeaponActor(Item);

	Item.bIsEquipped = false;
	InventoryItems[FreeIdx] = Item;
	EquippedItems.RemoveAt(EquipIdx);

	NotifyEquipChanged();
	return true;
}

bool ULB_InventoryComponent::SwapEquipSlots(const FLB_EquipSlotKey& A, const FLB_EquipSlotKey& B)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
		return false;

	// 같은 부위끼리만 (악세 0~3 사이). GE/무기 액터는 칸만 바뀌니까 그대로 둔다
	if (A == B || A.SlotTag != B.SlotTag || !IsValidEquipKey(A) || !IsValidEquipKey(B))
		return false;

	const int32 IdxA = FindEquippedIndex(A);
	const int32 IdxB = FindEquippedIndex(B);
	if (IdxA == INDEX_NONE && IdxB == INDEX_NONE)
		return false;

	if (IdxA != INDEX_NONE) EquippedItems[IdxA].SlotKey = B;
	if (IdxB != INDEX_NONE) EquippedItems[IdxB].SlotKey = A;

	NotifyEquipChanged();
	return true;
}

bool ULB_InventoryComponent::ReattachEquippedWeapon()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
		return false;

	// GE는 PlayerState의 ASC에 붙어 있어서 리스폰해도 유지된다. 무기 액터만 새 Pawn에 다시 붙이면 됨
	for (const FLB_EquippedItem& E : EquippedItems)
	{
		const ULB_DA_EquipInfo* Info = Cast<ULB_DA_EquipInfo>(GetItemInfo(E.Item.ItemCode));
		if (Info && Info->WeaponTag.IsValid() && !Info->EquipClass.IsNull())
		{
			ApplyWeaponActor(E.Item);
			return true;
		}
	}
	return false;
}

// ============================================================
// 장비창 UI 요청 (RPC)
// ============================================================

void ULB_InventoryComponent::Server_EquipToSlot_Implementation(int32 InventoryIndex, const FLB_EquipSlotKey& DstKey)
{
	EquipFromInventory(InventoryIndex, DstKey);
}

void ULB_InventoryComponent::Server_UnequipSlot_Implementation(const FLB_EquipSlotKey& Key)
{
	UnequipToInventory(Key);
}

void ULB_InventoryComponent::Server_UnequipToSlot_Implementation(const FLB_EquipSlotKey& Key, int32 DstInventoryIndex)
{
	UnequipToInventory(Key,DstInventoryIndex);
}

void ULB_InventoryComponent::Server_SwapEquipSlots_Implementation(const FLB_EquipSlotKey& A, const FLB_EquipSlotKey& B)
{
	SwapEquipSlots(A, B);
}

// ============================================================
// OnRep
// ============================================================

void ULB_InventoryComponent::OnRep_InventoryItems()
{
	UE_LOG(LogTemp, Warning, TEXT("[OnRep] Net=%d Slot0Stack=%d"),
	       (int32)GetNetMode(), InventoryItems.Num() > 0 ? InventoryItems[0].StackCount : -1);
	OnInventoryUpdated.Broadcast();
}

void ULB_InventoryComponent::OnRep_UnlockedSlots()
{
	OnInventoryUpdated.Broadcast();
}

void ULB_InventoryComponent::OnRep_EquippedItems()
{
	OnInventoryUpdated.Broadcast();
}

// ============================================================
// 내부: 공용 헬퍼
// ============================================================

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
		return RemoveItem(ItemInstanceId, 1); // Entry는 이후 사용 금지
	}
	return false;
}

// ============================================================
// 내부: 장비 슬롯
// ============================================================

int32 ULB_InventoryComponent::FindEquippedIndex(const FLB_EquipSlotKey& Key) const
{
	return EquippedItems.IndexOfByPredicate([&](const FLB_EquippedItem& E) { return E.SlotKey == Key; });
}

bool ULB_InventoryComponent::IsValidEquipKey(const FLB_EquipSlotKey& Key) const
{
	if (!Key.IsValid())
		return false;

	const int32 MaxIndex = Key.SlotTag.MatchesTagExact(TAG_Equip_Accessory) ? MaxAccessorySlots : 1;
	return Key.Index < MaxIndex;
}

bool ULB_InventoryComponent::FindFreeEquipKey(const FGameplayTag& SlotTag, FLB_EquipSlotKey& OutKey) const
{
	if (!SlotTag.IsValid())
		return false;

	const int32 Count = SlotTag.MatchesTagExact(TAG_Equip_Accessory) ? MaxAccessorySlots : 1;
	for (int32 i = 0; i < Count; ++i)
	{
		const FLB_EquipSlotKey Key(SlotTag, i);
		if (FindEquippedIndex(Key) == INDEX_NONE)
		{
			OutKey = Key;
			return true;
		}
	}

	OutKey = FLB_EquipSlotKey(SlotTag, 0); // 다 차 있으면 0번과 교체
	return true;
}

FGameplayTag ULB_InventoryComponent::GetEquipSlotTag(const ULB_DA_EquipInfo* Info) const
{
	if (!Info)
		return FGameplayTag();

	if (Info->EquipSlotTag.IsValid())
		return Info->EquipSlotTag;

	// 예전 무기 DA는 EquipSlotTag 없이 WeaponTag만 있어서 무기 칸으로 취급
	if (Info->WeaponTag.IsValid() && !Info->EquipClass.IsNull())
		return TAG_Equip_Weapon;

	return FGameplayTag();
}

// ============================================================
// 내부: 장비 효과 / 무기 액터
// ============================================================

void ULB_InventoryComponent::ApplyEquipEffects(const FLB_ItemEntry& Item)
{
	const ULB_DA_ItemInfo* Info = GetItemInfo(Item.ItemCode);
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!Info || !ASC)
		return;

	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	Ctx.AddSourceObject(Info);

	TArray<FActiveGameplayEffectHandle>& Handles = EquipEffectHandles.FindOrAdd(Item.ItemInstanceId);
	for (const TSubclassOf<UGameplayEffect>& GE : Info->Granted_GE)
	{
		if (!GE)
			continue;

		// 장비 GE는 Infinite여야 해제할 때 되돌릴 수 있다 (Instant면 핸들이 무효라 영구 적용됨)
		const FActiveGameplayEffectHandle Handle =
			ASC->ApplyGameplayEffectToSelf(GE->GetDefaultObject<UGameplayEffect>(), 1.f, Ctx);
		if (Handle.IsValid())
		{
			Handles.Add(Handle);
		}
	}
}

void ULB_InventoryComponent::RemoveEquipEffects(const FGuid& ItemInstanceId)
{
	TArray<FActiveGameplayEffectHandle> Handles;
	if (!EquipEffectHandles.RemoveAndCopyValue(ItemInstanceId, Handles))
		return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		for (const FActiveGameplayEffectHandle& Handle : Handles)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}
}

void ULB_InventoryComponent::ApplyWeaponActor(const FLB_ItemEntry& Item)
{
	const ULB_DA_EquipInfo* Info = Cast<ULB_DA_EquipInfo>(GetItemInfo(Item.ItemCode));
	if (!Info || !Info->WeaponTag.IsValid() || Info->EquipClass.IsNull())
		return; // 방어구/악세는 액터 없음

	const TSubclassOf<ALB_Equipment> EquipClass = Info->EquipClass.LoadSynchronous();
	ULB_EquipComponent* EquipComp = GetEquipComponent();
	if (EquipClass && EquipComp)
	{
		EquipComp->EquipWeaponSet(Info->WeaponTag, EquipClass);
	}
}

void ULB_InventoryComponent::RemoveWeaponActor(const FLB_ItemEntry& Item)
{
	const ULB_DA_EquipInfo* Info = Cast<ULB_DA_EquipInfo>(GetItemInfo(Item.ItemCode));
	if (!Info || !Info->WeaponTag.IsValid())
		return;

	if (ULB_EquipComponent* EquipComp = GetEquipComponent())
	{
		EquipComp->UnequipWeaponSet();
	}
}

void ULB_InventoryComponent::NotifyEquipChanged()
{
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
	// 서버(리슨서버 호스트)는 OnRep이 안 불려서 직접 브로드캐스트
	OnInventoryUpdated.Broadcast();
}
