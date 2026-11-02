#include "SLB_InventoryBox.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

// ---------- DragDropOp ----------
TSharedRef<FLB_SlotDragDropOp> FLB_SlotDragDropOp::Create(FName InGroup, int32 InIndex, const FSlateBrush* InIcon)
{
	TSharedRef<FLB_SlotDragDropOp> Op = MakeShared<FLB_SlotDragDropOp>();
	Op->SourceGroup = InGroup;
	Op->SourceIndex = InIndex;
	if (InIcon)
	{
		Op->IconBrush = *InIcon;
	}
	Op->Construct();
	return Op;
}

TSharedPtr<SWidget> FLB_SlotDragDropOp::GetDefaultDecorator() const
{
	return SNew(SBox)
		.WidthOverride(48.f)
		.HeightOverride(48.f)
		[
			SNew(SImage)
			.Image(&IconBrush)
			.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.7f))
		];
}

// ---------- SLB_InventoryBox ----------
void SLB_InventoryBox::Construct(const FArguments& InArgs)
{
	Group = InArgs._Group;
	SlotIndex = InArgs._SlotIndex;
	HotkeyTextValue = InArgs._HotkeyText;
	ItemIcon = InArgs._ItemIcon;
	ItemCount = InArgs._ItemCount;
	OnSlotDropped = InArgs._OnSlotDropped;
	OnUse = InArgs._OnUse;

	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	SLB_SquareBox::Construct(
		SLB_SquareBox::FArguments()
		.BoxSize(InArgs._BoxSize)
		.Selected(InArgs._Selected)
		[
			// 자식은 전부 HitTestInvisible -> 마우스 이벤트는 박스가 받음
			// 값은 람다로 매번 읽음 (세터로 바꿔도 반영되도록)
			SNew(SOverlay)
			+ SOverlay::Slot().Padding(4.f)
			[
				SNew(SImage)
				.Visibility(EVisibility::HitTestInvisible)
				.Image_Lambda([this]() { return ItemIcon.Get(nullptr); })
			]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 4.f, 2.f)
			[
				SNew(STextBlock)
				.Font(Font)
				.Text_Lambda([this]() { return FText::AsNumber(ItemCount.Get(0)); })
				.Visibility_Lambda([this]() { return ItemCount.Get(0) > 1 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(4.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Font(Font)
				.Text_Lambda([this]() { return HotkeyTextValue; })
				.Visibility_Lambda([this]() { return HotkeyTextValue.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
			]
		]);

	// 마우스 오버 정보창. 텍스트가 비면 툴팁 안 뜸.
	// 커스텀 정보창을 원하면 SetToolTip(SNew(SToolTip)[ ... ])로 교체
	SetToolTipText(InArgs._InfoText);
}

void SLB_InventoryBox::SetItem(const FSlateBrush& InBrush, int32 InCount)
{
	OwnedBrush = InBrush;
	ItemIcon.Set(&OwnedBrush);
	ItemCount.Set(InCount);
}

void SLB_InventoryBox::ClearItem()
{
	ItemIcon.Set(static_cast<const FSlateBrush*>(nullptr));
	ItemCount.Set(0);
}

void SLB_InventoryBox::SetInfoText(const FText& InText)
{
	SetToolTipText(InText);
}

FReply SLB_InventoryBox::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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
		TriggerUse();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SLB_InventoryBox::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasItem())
	{
		return FReply::Unhandled();
	}
	return FReply::Handled().BeginDragDrop(FLB_SlotDragDropOp::Create(Group, SlotIndex, ItemIcon.Get(nullptr)));
}

void SLB_InventoryBox::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if (DragDropEvent.GetOperationAs<FLB_SlotDragDropOp>().IsValid())
	{
		SetHighlighted(true);
	}
}

void SLB_InventoryBox::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	SetHighlighted(false);
}

FReply SLB_InventoryBox::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	SetHighlighted(false);

	const TSharedPtr<FLB_SlotDragDropOp> Op = DragDropEvent.GetOperationAs<FLB_SlotDragDropOp>();
	if (!Op.IsValid())
	{
		return FReply::Unhandled();
	}

	OnSlotDropped.ExecuteIfBound(Op->SourceGroup, Op->SourceIndex, SlotIndex);
	return FReply::Handled();
}