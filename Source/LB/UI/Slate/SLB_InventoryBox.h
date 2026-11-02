#pragma once

#include "CoreMinimal.h"
#include "Input/DragAndDrop.h"
#include "Styling/SlateBrush.h"
#include "SLB_SquareBox.h"

/** (원본 그룹, 원본 인덱스, 대상 인덱스) — 박스끼리 드래그앤드롭 결과 */
DECLARE_DELEGATE_ThreeParams(FLB_OnSlotDropped, FName /*SrcGroup*/, int32 /*SrcIndex*/, int32 /*DstIndex*/);
DECLARE_DELEGATE_OneParam(FLB_OnSlotUse, int32 /*SlotIndex*/);

/** 박스 간 드래그 정보. Group으로 인벤토리<->퀵슬롯 구분. 아이콘 브러시는 복사해서 보유 */
class LB_API FLB_SlotDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FLB_SlotDragDropOp, FDragDropOperation)

	FName SourceGroup;
	int32 SourceIndex = INDEX_NONE;
	FSlateBrush IconBrush;

	static TSharedRef<FLB_SlotDragDropOp> Create(FName InGroup, int32 InIndex, const FSlateBrush* InIcon);
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
};

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
	{}
		SLATE_ARGUMENT(FVector2D, BoxSize)
		SLATE_ARGUMENT(FName, Group)
		SLATE_ARGUMENT(int32, SlotIndex)
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
	FName Group;
	int32 SlotIndex = INDEX_NONE;
	FText HotkeyTextValue;
	FSlateBrush OwnedBrush;
	TAttribute<const FSlateBrush*> ItemIcon;
	TAttribute<int32> ItemCount;
	FLB_OnSlotDropped OnSlotDropped;
	FLB_OnSlotUse OnUse;
};