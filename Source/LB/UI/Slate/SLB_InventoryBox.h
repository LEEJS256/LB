#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "SLB_SquareBox.h"
#include "UI/UI_Utility/LB_SlotDragDropOp.h"

/**
 * 아이템 한 칸. 아이콘/수량/(선택)단축키 라벨 표시.
 * - 좌클릭 드래그: 다른 InventoryBox/QuickSlot으로 이동
 * - 우클릭: OnUse
 * - 마우스 올리면 InfoText 툴팁 (빈 텍스트면 안 뜸)
 * 데이터는 모름. 표시용 값만 받음 (SetItem / ClearItem).
 */
class LB_API SLB_InventoryBox : public SLB_SquareBox
{
public:
	SLATE_BEGIN_ARGS(SLB_InventoryBox)
			: _BoxSize(FVector2D(64.f, 64.f))
			  , _Group(TEXT("Inventory"))
			  , _SlotIndex(INDEX_NONE)
			  , _CountTextFont()
			  , _HotkeyTextFont()
		{
		}

		SLATE_ARGUMENT(FName, Group)
		SLATE_ARGUMENT(int32, SlotIndex)
		SLATE_ARGUMENT(FSlateFontInfo, CountTextFont)
		SLATE_ARGUMENT(FSlateFontInfo, HotkeyTextFont)
		SLATE_ARGUMENT(FVector2D, BoxSize)
		SLATE_ARGUMENT(FText, HotkeyText)
		SLATE_ATTRIBUTE(const FSlateBrush*, ItemIcon)
		SLATE_ATTRIBUTE(int32, ItemCount)
		SLATE_ATTRIBUTE(FText, InfoText)
		SLATE_ATTRIBUTE(bool, Selected)
		SLATE_EVENT(FLB_OnSlotDropped, OnSlotDropped)
		SLATE_EVENT(FLB_OnSlotUse, OnUse)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ---- 런타임 세터 (UMG 래퍼가 사용) ----
	void SetItem(const FSlateBrush& InBrush, int32 InCount);
	void ClearItem();
	void SetInfoText(const FText& InText);
	void SetHotkeyText(const FText& InText) { HotkeyTextValue = InText; }
	void SetSlotIndex(int32 InIndex) { SlotIndex = InIndex; }
	void SetGroup(FName InGroup) { Group = InGroup; }

	void SetTestCount(int32 InTestCount);
	void ClearTestCount();

	/** 단축키/외부에서 사용 트리거 */
	void TriggerUse() const { OnUse.ExecuteIfBound(SlotIndex); }

	bool HasItem() const { return ItemIcon.Get(nullptr) != nullptr; }
	int32 GetSlotIndex() const { return SlotIndex; }
	FName GetGroup() const { return Group; }

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

private:
	int32 TestCount = 0;
	bool HasTestCount = false; // 테스트 값이 설정되었는지 여부

	FName Group;
	int32 SlotIndex = INDEX_NONE;
	FText HotkeyTextValue;
	FSlateBrush OwnedBrush;
	TAttribute<const FSlateBrush*> ItemIcon;
	TAttribute<int32> ItemCount;
	FLB_OnSlotDropped OnSlotDropped;
	FLB_OnSlotUse OnUse;
};
