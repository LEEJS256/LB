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

	const FSlateFontInfo CountFont = InArgs._CountTextFont.HasValidFont()
		                                 ? InArgs._CountTextFont
		                                 : FCoreStyle::GetDefaultFontStyle("Bold", 10);
	const FSlateFontInfo HotkeyFont = InArgs._HotkeyTextFont.HasValidFont()
		                                  ? InArgs._HotkeyTextFont
		                                  : FCoreStyle::GetDefaultFontStyle("Bold", 10);

	SLB_SquareBox::Construct(
		SLB_SquareBox::FArguments()
		.BoxSize(InArgs._BoxSize)
		.Selected(InArgs._Selected)
		[
			SNew(SOverlay)
			+ SOverlay::Slot().Padding(4.f)
			[
				SNew(SImage)
				.Visibility(EVisibility::HitTestInvisible)
				.Image_Lambda([this]() { return ItemIcon.Get(nullptr); })
			]
			// SLB_InventoryBox.cpp 의 Construct 함수 내부 중 수량 텍스트 부분 수정
			// SLB_InventoryBox.cpp 의 Construct 함수 내부

			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 4.f, 2.f)
			[
				SNew(STextBlock)
				.Font(CountFont)
				.Text_Lambda([this]()
				{
					// [수정] 테스트 값이 있더라도, 실제 아이템이 존재하고(ItemCount가 유효하고) 
					// 인게임 상황이라면 실제 ItemCount를 우선하거나, 
					// 혹은 테스트 모드가 확실히 켜져있을 때만 TestCount를 쓰도록 분기
					int32 DisplayCount = (HasTestCount && ItemCount.Get(0) <= 0) ? TestCount : ItemCount.Get(0);

					// 만약 에디터 미리보기(TestCount)를 무조건 최우선으로 보고 싶으셨던 거라면, 
					// 아이템을 먹었을 때(인게임 갱신 시) TestCount를 Clear해주는 로직이 필요합니다.
					// 아래처럼 실제 아이템 개수가 들어오면 TestCount보다 ItemCount를 쓰게 하는 것이 안전합니다.

					return FText::AsNumber(ItemCount.Get(0) > 0 ? ItemCount.Get(0) : (HasTestCount ? TestCount : 0));
				})
				.Visibility_Lambda([this]()
				{
					int32 RealCount = ItemCount.Get(0);
					int32 DisplayCount = RealCount > 0 ? RealCount : (HasTestCount ? TestCount : 0);
					return DisplayCount > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
				})
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(4.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				                .Font(HotkeyFont) // [+] 단축키 폰트 적용
				                .Text_Lambda([this]() { return HotkeyTextValue; })
				                .Visibility_Lambda([this]()
				                {
					                return HotkeyTextValue.IsEmpty()
						                       ? EVisibility::Collapsed
						                       : EVisibility::HitTestInvisible;
				                })
			]

		]);

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

void SLB_InventoryBox::SetTestCount(int32 InTestCount)
{
	TestCount = InTestCount;
	HasTestCount = true;
}

void SLB_InventoryBox::ClearTestCount()
{
	HasTestCount = false;
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
