#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "SLB_SquareBox.h"
#include "UI/UI_Utility/LB_EquipSlotKey.h"
#include "UI/UI_Utility/LB_SlotDragDropOp.h"

/** (대상 장비 슬롯, 원본 그룹, 원본 인덱스) — 인벤토리 등에서 이 칸으로 드롭됨 */
DECLARE_DELEGATE_ThreeParams(FLB_OnEquipSlotDropped, const FLB_EquipSlotKey& /*DstKey*/, FName /*SrcGroup*/, int32 /*SrcIndex*/);
/** 우클릭 해제 */
DECLARE_DELEGATE_OneParam(FLB_OnEquipSlotUnequip, const FLB_EquipSlotKey& /*Key*/);
/** 드롭 가능 여부 (하이라이트용. 최종 판정은 서버) */
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FLB_CanAcceptEquipDrop, const FLB_EquipSlotKey& /*DstKey*/, FName /*SrcGroup*/, int32 /*SrcIndex*/);

/**
 * 장비 한 칸 (Slate). SLB_SquareBox 상속.
 * - 장착 아이콘 표시, 비어 있으면 EmptyBrush(실루엣)를 흐리게 표시
 * - 좌클릭 드래그: FLB_SlotDragDropOp(Group="Equip", SlotIndex)로 끌어냄 → 인벤토리에 드롭하면 해제
 * - 우클릭: OnUnequip
 * - 드롭 받기: OnEquipDropped(SlotKey, SrcGroup, SrcIndex)
 * 데이터는 모름. 표시용 값만 받음 (SetItem / ClearItem).
 */
class LB_API SLB_EquipSlot : public SLB_SquareBox
{
public:
	SLATE_BEGIN_ARGS(SLB_EquipSlot)
		: _BoxSize(FVector2D(64.f, 64.f))
		, _Group(TEXT("Equip"))
		, _SlotIndex(INDEX_NONE)
	{}
		SLATE_ARGUMENT(FVector2D, BoxSize)
		SLATE_ARGUMENT(FLB_EquipSlotKey, SlotKey)
		/** 드래그 소스 구분용 그룹 이름 */
		SLATE_ARGUMENT(FName, Group)
		/** 장비창 안에서의 고유 번호 (드래그 SourceIndex로 사용) */
		SLATE_ARGUMENT(int32, SlotIndex)
		SLATE_EVENT(FLB_OnEquipSlotDropped, OnEquipDropped)
		SLATE_EVENT(FLB_OnEquipSlotUnequip, OnUnequip)
		SLATE_EVENT(FLB_CanAcceptEquipDrop, CanAcceptDrop)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ---- 런타임 세터 (UMG 래퍼가 사용) ----
	void SetItem(const FSlateBrush& InBrush);
	void ClearItem();
	void SetEmptyBrush(const FSlateBrush& InBrush);
	void ClearEmptyBrush();
	void SetSlotKey(const FLB_EquipSlotKey& InKey) { SlotKey = InKey; }
	void SetSlotIndex(int32 InIndex) { SlotIndex = InIndex; }
	void SetGroup(FName InGroup) { Group = InGroup; }

	bool HasItem() const { return bHasItem; }
	const FLB_EquipSlotKey& GetSlotKey() const { return SlotKey; }
	int32 GetSlotIndex() const { return SlotIndex; }
	FName GetGroup() const { return Group; }

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	const FSlateBrush* GetDisplayBrush() const;
	FSlateColor GetIconColor() const;
	bool CanAccept(const TSharedPtr<FLB_SlotDragDropOp>& Op) const;

	FLB_EquipSlotKey SlotKey;
	FName Group;
	int32 SlotIndex = INDEX_NONE;

	FSlateBrush ItemBrush;
	FSlateBrush EmptyBrush;
	bool bHasItem = false;
	bool bHasEmptyBrush = false;

	FLB_OnEquipSlotDropped OnEquipDropped;
	FLB_OnEquipSlotUnequip OnUnequip;
	FLB_CanAcceptEquipDrop CanAcceptDrop;
};
