#pragma once

#include "CoreMinimal.h"
#include "LB_QuickSlotTextStyle.h"
#include "SLB_SquareBox.h"
#include "SLB_InventoryBox.h" // FLB_SlotDragDropOp, FLB_OnSlotDropped 재사용

DECLARE_DELEGATE(FLB_OnQuickSlotClear);

/**
 * 퀵슬롯 한 칸 (Slate). SLB_SquareBox 상속.
 * - 아이콘 / 쿨타임 오버레이 / 단축키 라벨(좌상) / 수량(우하) 표시. 데이터는 모르고 표시값만 받음.
 * - 좌클릭 드래그: FLB_SlotDragDropOp(Group, SlotIndex)로 다른 칸에 이동
 * - 드롭: OnSlotDropped(SrcGroup, SrcIndex, DstIndex=내 SlotIndex) → 실제 처리는 바깥(패널)에서
 * - 우클릭: OnClearRequested (슬롯 비우기)
 */
class LB_API SLB_QuickSlot : public SLB_SquareBox
{
public:
	SLATE_BEGIN_ARGS(SLB_QuickSlot)
			: _BoxSize(FVector2D(64.f, 64.f))
			  , _Group(TEXT("QuickItem"))
			  , _SlotIndex(INDEX_NONE)
			  , _HotkeyStyle(FLB_QuickSlotTextStyle::HotkeyDefault())
			  , _CountStyle(FLB_QuickSlotTextStyle::CountDefault())
		{
		}

		SLATE_ARGUMENT(FName, Group)
		SLATE_ARGUMENT(int32, SlotIndex)
		SLATE_ARGUMENT(FLB_QuickSlotTextStyle, HotkeyStyle)
		SLATE_ARGUMENT(FLB_QuickSlotTextStyle, CountStyle)
		SLATE_ARGUMENT(FVector2D, BoxSize)
		SLATE_ARGUMENT(FText, HotkeyText)
		SLATE_ATTRIBUTE(FText, InfoText)
		SLATE_ATTRIBUTE(bool, Selected)
		SLATE_EVENT(FLB_OnSlotDropped, OnSlotDropped)
		SLATE_EVENT(FLB_OnQuickSlotClear, OnClearRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ---- 런타임 세터 (UMG 래퍼가 사용) ----
	void SetItem(const FSlateBrush& InBrush, int32 InCount); // 수량 0이면 숫자 숨김 (스킬 칸)
	void ClearItem();
	void SetDimmed(bool bInDimmed) { bDimmed = bInDimmed; } // 등록은 돼 있는데 수량 0일 때 흐리게
	void SetCooldownPercent(float InPercent) { CooldownPercent = InPercent; } // 0 = 없음, 1 = 방금 사용
	void SetInfoText(const FText& InText);
	void SetHotkeyText(const FText& InText) { HotkeyTextValue = InText; }
	void SetHotkeyStyle(const FLB_QuickSlotTextStyle& InStyle); // 폰트/색/정렬/위치 (런타임·디자이너 갱신용)
	void SetCountStyle(const FLB_QuickSlotTextStyle& InStyle);
	void SetSlotIndex(int32 InIndex) { SlotIndex = InIndex; }
	void SetGroup(FName InGroup) { Group = InGroup; }

	bool HasItem() const { return bHasIcon; }
	int32 GetSlotIndex() const { return SlotIndex; }
	FName GetGroup() const { return Group; }

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

private:
	FName Group;
	int32 SlotIndex = INDEX_NONE;
	FText HotkeyTextValue;

	FLB_QuickSlotTextStyle HotkeyStyle;
	FLB_QuickSlotTextStyle CountStyle;
	SOverlay::FOverlaySlot* HotkeySlot = nullptr; // 위치(정렬/패딩)를 나중에 바꾸기 위해 보관
	SOverlay::FOverlaySlot* CountSlot = nullptr;

	FSlateBrush OwnedBrush;
	bool bHasIcon = false;
	int32 Count = 0;
	bool bDimmed = false;
	float CooldownPercent = 0.f;

	FLB_OnSlotDropped OnSlotDropped;
	FLB_OnQuickSlotClear OnClearRequested;
};
