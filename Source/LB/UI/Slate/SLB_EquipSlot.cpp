#include "SLB_EquipSlot.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"

void SLB_EquipSlot::Construct(const FArguments& InArgs)
{
	SlotKey = InArgs._SlotKey;
	Group = InArgs._Group;
	SlotIndex = InArgs._SlotIndex;
	OnEquipDropped = InArgs._OnEquipDropped;
	OnUnequip = InArgs._OnUnequip;
	CanAcceptDrop = InArgs._CanAcceptDrop;

	SLB_SquareBox::Construct(
		SLB_SquareBox::FArguments()
		.BoxSize(InArgs._BoxSize)
		[
			SNew(SOverlay)
			+ SOverlay::Slot().Padding(4.f)
			[
				SNew(SImage)
				.Visibility(EVisibility::HitTestInvisible)
				.Image(this, &SLB_EquipSlot::GetDisplayBrush)
				.ColorAndOpacity(this, &SLB_EquipSlot::GetIconColor)
			]
		]);
}

// ---------- 표시 ----------

void SLB_EquipSlot::SetItem(const FSlateBrush& InBrush)
{
	ItemBrush = InBrush;
	bHasItem = true;
}

void SLB_EquipSlot::ClearItem()
{
	bHasItem = false;
}

void SLB_EquipSlot::SetEmptyBrush(const FSlateBrush& InBrush)
{
	EmptyBrush = InBrush;
	bHasEmptyBrush = true;
}

void SLB_EquipSlot::ClearEmptyBrush()
{
	bHasEmptyBrush = false;
}

const FSlateBrush* SLB_EquipSlot::GetDisplayBrush() const
{
	if (bHasItem) return &ItemBrush;
	if (bHasEmptyBrush) return &EmptyBrush;
	return nullptr;
}

FSlateColor SLB_EquipSlot::GetIconColor() const
{
	// 빈 칸 실루엣은 흐리게
	return FSlateColor(bHasItem ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.35f));
}

// ---------- 마우스 ----------

FReply SLB_EquipSlot::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasItem())
	{
		return FReply::Unhandled();
	}
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnUnequip.ExecuteIfBound(SlotKey);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SLB_EquipSlot::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasItem())
	{
		return FReply::Unhandled();
	}
	TSharedRef<FLB_SlotDragDropOp> Op = FLB_SlotDragDropOp::Create(Group, SlotIndex, &ItemBrush);
	Op->EquipKey = SlotKey;
	return FReply::Handled().BeginDragDrop(Op);
}

// ---------- 드롭 ----------

bool SLB_EquipSlot::CanAccept(const TSharedPtr<FLB_SlotDragDropOp>& Op) const
{
	if (!Op.IsValid())
	{
		return false;
	}
	// 자기 자신에게 드롭은 무시
	if (Op->SourceGroup == Group && Op->SourceIndex == SlotIndex)
	{
		return false;
	}
	return CanAcceptDrop.IsBound() ? CanAcceptDrop.Execute(SlotKey, Op->SourceGroup, Op->SourceIndex) : true;
}

void SLB_EquipSlot::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	SetHighlighted(CanAccept(DragDropEvent.GetOperationAs<FLB_SlotDragDropOp>()));
}

void SLB_EquipSlot::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	SetHighlighted(false);
}

FReply SLB_EquipSlot::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	SetHighlighted(false);

	const TSharedPtr<FLB_SlotDragDropOp> Op = DragDropEvent.GetOperationAs<FLB_SlotDragDropOp>();
	if (!CanAccept(Op))
	{
		return FReply::Unhandled();
	}

	OnEquipDropped.ExecuteIfBound(SlotKey, Op->SourceGroup, Op->SourceIndex);
	return FReply::Handled();
}

FReply SLB_EquipSlot::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasItem() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnUnequip.ExecuteIfBound(SlotKey);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
