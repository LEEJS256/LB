#include "SLB_QuickSlot.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// 폰트가 지정 안 돼 있으면 기본 Bold 10
	FSlateFontInfo ResolveFont(const FLB_QuickSlotTextStyle& Style)
	{
		return Style.Font.HasValidFont() ? Style.Font : FCoreStyle::GetDefaultFontStyle("Bold", 10);
	}
}

void SLB_QuickSlot::Construct(const FArguments& InArgs)
{
	Group = InArgs._Group;
	SlotIndex = InArgs._SlotIndex;
	HotkeyTextValue = InArgs._HotkeyText;
	OnSlotDropped = InArgs._OnSlotDropped;
	OnClearRequested = InArgs._OnClearRequested;
	HotkeyStyle = InArgs._HotkeyStyle;
	CountStyle = InArgs._CountStyle;

	SLB_SquareBox::Construct(
		SLB_SquareBox::FArguments()
		.BoxSize(InArgs._BoxSize)
		.Selected(InArgs._Selected)
		[
			SNew(SOverlay)

			// 아이콘 (수량 0이면 흐리게)
			+ SOverlay::Slot().Padding(4.f)
				[
					SNew(SImage)
					.Visibility(EVisibility::HitTestInvisible)
					.Image_Lambda([this]() -> const FSlateBrush* { return bHasIcon ? &OwnedBrush : nullptr; })
					.ColorAndOpacity_Lambda([this]()
					{
						return bDimmed ? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor::White;
					})
				]

			// 쿨타임 오버레이 (아래에서 위로 줄어드는 반투명 막)
			+ SOverlay::Slot().Padding(4.f)
			[
				SNew(SImage)
				.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.ColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
				.RenderTransformPivot(FVector2D(0.5f, 1.f))
				.RenderTransform_Lambda([this]() -> TOptional<FSlateRenderTransform>
				{
					return FSlateRenderTransform(FScale2D(1.f, FMath::Clamp(CooldownPercent, 0.f, 1.f)));
				})
				.Visibility_Lambda([this]()
				{
					return CooldownPercent > 0.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
				})
			]

			// 단축키 라벨 (기본 좌상단, HotkeyStyle로 위치/글꼴 변경)
			+ SOverlay::Slot()
			.Expose(HotkeySlot)
			.HAlign(HotkeyStyle.HAlign)
			.VAlign(HotkeyStyle.VAlign)
			.Padding(HotkeyStyle.Padding)
			[
				SNew(STextBlock)
				.Font_Lambda([this]() { return ResolveFont(HotkeyStyle); })
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(HotkeyStyle.Color); })
				.Justification_Lambda([this]() { return HotkeyStyle.Justification.GetValue(); })
				.Text_Lambda([this]() { return HotkeyTextValue; })
				.Visibility_Lambda([this]()
				{
					return HotkeyTextValue.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
				})
			]

			// 수량 (기본 우하단, CountStyle로 위치/글꼴 변경)
			+ SOverlay::Slot()
			.Expose(CountSlot)
			.HAlign(CountStyle.HAlign)
			.VAlign(CountStyle.VAlign)
			.Padding(CountStyle.Padding)
			[
				SNew(STextBlock)
				.Font_Lambda([this]() { return ResolveFont(CountStyle); })
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(CountStyle.Color); })
				.Justification_Lambda([this]() { return CountStyle.Justification.GetValue(); })
				.Text_Lambda([this]() { return FText::AsNumber(Count); })
				.Visibility_Lambda([this]()
				{
					return Count > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
				})
			]
		]);
	
	// SetToolTipText(InArgs._InfoText);
}

void SLB_QuickSlot::SetItem(const FSlateBrush& InBrush, int32 InCount)
{
	OwnedBrush = InBrush;
	bHasIcon = true;
	Count = InCount;
}

void SLB_QuickSlot::ClearItem()
{
	bHasIcon = false;
	Count = 0;
}

void SLB_QuickSlot::SetInfoText(const FText& InText)
{
	// SetToolTipText(InText);
}

void SLB_QuickSlot::SetHotkeyStyle(const FLB_QuickSlotTextStyle& InStyle)
{
	HotkeyStyle = InStyle;
	if (HotkeySlot)
	{
		HotkeySlot->SetHorizontalAlignment(InStyle.HAlign);
		HotkeySlot->SetVerticalAlignment(InStyle.VAlign);
		HotkeySlot->SetPadding(InStyle.Padding);
	}
}

void SLB_QuickSlot::SetCountStyle(const FLB_QuickSlotTextStyle& InStyle)
{
	CountStyle = InStyle;
	if (CountSlot)
	{
		CountSlot->SetHorizontalAlignment(InStyle.HAlign);
		CountSlot->SetVerticalAlignment(InStyle.VAlign);
		CountSlot->SetPadding(InStyle.Padding);
	}
}

FReply SLB_QuickSlot::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnClearRequested.ExecuteIfBound(); // 우클릭 = 슬롯 비우기
		return FReply::Handled();
	}
 
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasItem())
	{
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}
	return FReply::Unhandled();
}

FReply SLB_QuickSlot::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasItem())
	{
		return FReply::Unhandled();
	}
	return FReply::Handled().BeginDragDrop(FLB_SlotDragDropOp::Create(Group, SlotIndex, &OwnedBrush));
}

void SLB_QuickSlot::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if (DragDropEvent.GetOperationAs<FLB_SlotDragDropOp>().IsValid())
	{
		SetHighlighted(true);
	}
}

void SLB_QuickSlot::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	SetHighlighted(false);
}

FReply SLB_QuickSlot::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	SetHighlighted(false);

	const TSharedPtr<FLB_SlotDragDropOp> Op = DragDropEvent.GetOperationAs<FLB_SlotDragDropOp>();
	if (!Op.IsValid())
	{
		return FReply::Unhandled();
	}

	// 자기 자신 위에 놓은 건 무시
	if (Op->SourceGroup == Group && Op->SourceIndex == SlotIndex)
	{
		return FReply::Handled();
	}

	OnSlotDropped.ExecuteIfBound(Op->SourceGroup, Op->SourceIndex, SlotIndex);
	return FReply::Handled();
}
