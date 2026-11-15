#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UI_Utility/LB_EquipSlotKey.h"
#include "LB_EquipPanel.generated.h"

class UImage;
class ULB_EquipSlot;
class ULB_InventoryComponent;
class ULB_DA_EquipInfo;

/**
 * 장비창 (인벤토리 왼쪽에 붙는 패널)
 *
 * WBP 구성
 *  - ULB_EquipSlot 9개를 디자이너에서 자유롭게 배치 (좌: 방어구 4 / 우: 악세 4 + 무기 1)
 *    → 각 칸의 SlotKey / SlotIndex 는 디테일 패널에서 지정. 이름은 상관없음 (자동 수집)
 *  - (선택) 가운데 캐릭터 프리뷰용 UImage, 이름 CharacterPreview
 *
 * 역할
 *  - 슬롯 이벤트(드롭/우클릭/드롭 가능 판정)를 받아서 인벤토리 컴포넌트에 요청
 *  - RefreshFromEquipped 로 받은 장착 상태를 칸에 표시
 *  - 데이터는 들고 있지 않음 (장착 상태의 원본은 컴포넌트)
 */
UCLASS(Abstract)
class LB_API ULB_EquipPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 장착 상태 표시 갱신. Key → ItemCode (없는 키는 빈 칸) */
	void RefreshFromEquipped(const TMap<FLB_EquipSlotKey, FName>& Equipped);

	/** 슬롯 키로 칸 찾기 (없으면 nullptr) */
	ULB_EquipSlot* FindSlot(const FLB_EquipSlotKey& Key) const;

	/** 드래그 SourceIndex(SlotIndex) → 슬롯 키. 인벤토리 패널이 "Equip" 드롭을 받을 때 사용 */
	bool GetSlotKeyByIndex(int32 SlotIndex, FLB_EquipSlotKey& OutKey) const;

	static FName EquipGroup() { return FName(TEXT("Equip")); }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 캐릭터 프리뷰 자리 (RenderTarget은 나중에 연결) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterPreview;

private:
	// ---- 슬롯 수집 / 바인딩 ----
	void CollectSlots();

	UFUNCTION()
	void HandleEquipDropped(FLB_EquipSlotKey DstKey, FName SrcGroup, int32 SrcIndex);

	UFUNCTION()
	void HandleUnequip(FLB_EquipSlotKey Key);

	bool HandleCanAccept(const FLB_EquipSlotKey& DstKey, FName SrcGroup, int32 SrcIndex) const;

	// ---- 인벤토리 연동 ----
	void TryBindInventory();

	UFUNCTION()
	void HandleInventoryUpdated();

	/** 인벤토리 칸 번호 → 아이템 코드 */
	FName ResolveInventoryItemCode(int32 InventoryIndex) const;

	/** 아이템 코드 → 장비 DA (장비가 아니면 nullptr) */
	const ULB_DA_EquipInfo* ResolveEquipInfo(FName ItemCode) const;

	// ---- 컴포넌트 요청 (서버 RPC 연결 지점) ----
	void RequestEquip(int32 InventoryIndex, const FLB_EquipSlotKey& DstKey);
	void RequestUnequip(const FLB_EquipSlotKey& Key);
	void RequestSwap(const FLB_EquipSlotKey& A, const FLB_EquipSlotKey& B);

	UPROPERTY(Transient)
	TMap<FLB_EquipSlotKey, TObjectPtr<ULB_EquipSlot>> SlotByKey;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ULB_EquipSlot>> SlotByIndex;

	TWeakObjectPtr<ULB_InventoryComponent> Inventory;
	FTimerHandle BindRetryHandle;
};