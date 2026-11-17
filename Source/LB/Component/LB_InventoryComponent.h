// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"            // FActiveGameplayEffectHandle
#include "UI/UI_Utility/LB_EquipSlotKey.h"
#include "Utility/LB_ItemEntry.h"
#include "LB_InventoryComponent.generated.h"

class ALB_DropItem;
class ULB_DA_ItemInfo;
class ULB_DA_EquipInfo;
class ULB_EquipComponent;

/** 장비창 한 칸에 장착된 아이템. 장착하면 인벤토리 배열에서 빠져서 여기로 옮겨진다 */
USTRUCT(BlueprintType)
struct FLB_EquippedItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equip")
	FLB_EquipSlotKey SlotKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equip")
	FLB_ItemEntry Item;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

/**
 * 인벤토리 + 장비 보유 컴포넌트 (PlayerState 소유)
 *
 * public
 *   1. 생명주기 / 이벤트
 *   2. 조회            : UI가 읽는 함수 (const)
 *   3. 인벤토리 조작    : 서버 전용 (AddItem 등)
 *   4. 인벤토리 RPC     : UI → 서버 요청 (사용/이동/정렬/버리기)
 *   5. 장비 조작        : 서버 전용 (장착/해제/교환)
 *   6. 장비창 RPC       : UI → 서버 요청
 * protected
 *   설정값 / 복제 데이터 / OnRep
 * private
 *   내부 헬퍼 / 장비 슬롯 / 장비 효과 / 서버 전용 런타임 데이터
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LB_API ULB_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// ==========================================
	// 1. 생명주기 / 이벤트
	// ==========================================

	ULB_InventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

	// ==========================================
	// 2. 조회
	// ==========================================

	/** 인벤토리 전체 항목 조회 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FLB_ItemEntry>& GetInventoryItems() const { return InventoryItems; }

	/** 현재 해금된 슬롯 수 */
	int32 Get_CurrentSlot() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemCode) const;

	// 첫 번째 엔트리의 GUID. 퀵슬롯에서 ItemCode로 사용할 때 필요
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool FindFirstEntryByCode(FName ItemCode, FGuid& OutId) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Equip")
	const TArray<FLB_EquippedItem>& GetEquippedItems() const { return EquippedItems; }

	/** 장비창 표시용: 슬롯 키 → 아이템 코드 */
	void GetEquippedMap(TMap<FLB_EquipSlotKey, FName>& Out) const;

	// ==========================================
	// 3. 인벤토리 조작 (서버 전용)
	// ==========================================

	/** 못 넣은 개수를 반환 (0이면 전부 성공) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(FName ItemCode, int32 Count = 1);

	/** 인스턴스 ID 기준 아이템 수량 차감/제거 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FGuid ItemInstanceId, int32 Count = 1);

	void ExpandInventory(int32 AddCount);

	// ==========================================
	// 4. 인벤토리 RPC (UI → 서버)
	// ==========================================

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_UseItem(FGuid ItemInstanceId);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_MoveItem(int32 SrcIndex, int32 DstIndex);

	/** 수량 많은 순으로 실제 배열을 정렬 (빈 칸은 뒤로). 같은 수량이면 아이템 코드 순 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_SortItems();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_DropItem(FGuid ItemInstanceId, int32 Count);

	// ==========================================
	// 5. 장비 조작 (서버 전용)
	// ==========================================

	/** 인벤토리 아이템을 맞는 장비칸에 자동 장착. 우클릭 사용 / 시작 무기용 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equip")
	bool EquipItem(FGuid ItemInstanceId);

	/** 장착 중인 아이템을 인벤토리 빈 칸으로 해제 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equip")
	bool UnequipItem(FGuid ItemInstanceId);

	/** 실제 처리 (RPC와 EquipItem/UnequipItem이 공용으로 사용) */
	bool EquipFromInventory(int32 InventoryIndex, const FLB_EquipSlotKey& DstKey);
	bool UnequipToInventory(const FLB_EquipSlotKey& Key, int32 PreferredIndex = INDEX_NONE);
	bool SwapEquipSlots(const FLB_EquipSlotKey& A, const FLB_EquipSlotKey& B);

	/** 리스폰 후 새 Pawn에 장착 중인 무기 액터를 다시 붙인다. 장착 무기가 있으면 true */
	bool ReattachEquippedWeapon();

	// ==========================================
	// 6. 장비창 RPC (UI → 서버)
	// ==========================================

	/** 인벤토리 칸 → 장비칸. 칸이 차 있으면 기존 장비와 교체 */
	UFUNCTION(Server, Reliable, Category = "Inventory|Equip")
	void Server_EquipToSlot(int32 InventoryIndex, const FLB_EquipSlotKey& DstKey);

	/** 장비칸 → 인벤토리 빈 칸 (우클릭 해제) */
	UFUNCTION(Server, Reliable, Category = "Inventory|Equip")
	void Server_UnequipSlot(const FLB_EquipSlotKey& Key);

	/** 장비칸 → 인벤토리 특정 칸 (드래그 해제) */
	UFUNCTION(Server, Reliable, Category = "Inventory|Equip")
	void Server_UnequipToSlot(const FLB_EquipSlotKey& Key, int32 DstInventoryIndex);

	/** 같은 종류 장비칸끼리 교환 (악세 0~3) */
	UFUNCTION(Server, Reliable, Category = "Inventory|Equip")
	void Server_SwapEquipSlots(const FLB_EquipSlotKey& A, const FLB_EquipSlotKey& B);

protected:
	// ==========================================
	// 설정값 (에디터)
	// ==========================================

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int32 MaxSlots = 90;

	/** 악세사리 칸 수 (Equip.Accessory 0 ~ N-1). 나머지 부위는 1칸 */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Equip", meta = (ClampMin = "1"))
	int32 MaxAccessorySlots = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<ALB_DropItem> DropItemClass;

	// ==========================================
	// 복제 데이터
	// ==========================================

	UPROPERTY(ReplicatedUsing = OnRep_InventoryItems, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FLB_ItemEntry> InventoryItems;

	/** 현재 해금된 슬롯 수 (기본값은 에디터에서) */
	UPROPERTY(ReplicatedUsing = OnRep_UnlockedSlots, EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 CurrentSlots = 35;

	/** 장착 중인 아이템들 (TMap은 복제가 안 돼서 배열) */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Equip")
	TArray<FLB_EquippedItem> EquippedItems;

	// ==========================================
	// OnRep
	// ==========================================

	UFUNCTION()
	void OnRep_InventoryItems();

	UFUNCTION()
	void OnRep_UnlockedSlots();

	UFUNCTION()
	void OnRep_EquippedItems();

private:
	// ---- 공용 헬퍼 ----
	const ULB_DA_ItemInfo* GetItemInfo(FName ItemCode) const;
	ULB_EquipComponent* GetEquipComponent() const;
	bool UseItem(FGuid ItemInstanceId);   // Server_UseItem 내부 처리

	// ---- 장비 슬롯 ----
	int32 FindEquippedIndex(const FLB_EquipSlotKey& Key) const;
	bool IsValidEquipKey(const FLB_EquipSlotKey& Key) const;
	/** 해당 부위의 빈 칸 (다 차 있으면 0번 칸 = 교체) */
	bool FindFreeEquipKey(const FGameplayTag& SlotTag, FLB_EquipSlotKey& OutKey) const;
	/** DA의 EquipSlotTag. 비어 있는 무기 DA는 Equip.Weapon으로 취급 */
	FGameplayTag GetEquipSlotTag(const ULB_DA_EquipInfo* Info) const;

	// ---- 장비 효과 / 무기 액터 ----
	void ApplyEquipEffects(const FLB_ItemEntry& Item);
	void RemoveEquipEffects(const FGuid& ItemInstanceId);
	void ApplyWeaponActor(const FLB_ItemEntry& Item);
	void RemoveWeaponActor(const FLB_ItemEntry& Item);
	void NotifyEquipChanged();

	// ---- 서버 전용 런타임 데이터 (복제 안 함) ----
	/** 장비가 적용한 Infinite GE 핸들 (해제할 때 제거용). 키 = ItemInstanceId */
	TMap<FGuid, TArray<FActiveGameplayEffectHandle>> EquipEffectHandles;
};
