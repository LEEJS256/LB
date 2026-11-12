#include "SLB_SquareBox.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SToolTip.h"
#include "Components/Widget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"

void SLB_SquareBox::Construct(const FArguments& InArgs)
{
	NormalColor = InArgs._NormalColor;
	HoverColor = InArgs._HoverColor;
	PressedColor = InArgs._PressedColor;
	SelectedColor = InArgs._SelectedColor;
	HighlightColor = InArgs._HighlightColor;
	SelectedAttr = InArgs._Selected;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	ChildSlot
	[
		SAssignNew(SizeBox, SBox)
		.WidthOverride(InArgs._BoxSize.X)
		.HeightOverride(InArgs._BoxSize.Y)
		[
			// 바깥: 테두리
			SAssignNew(OuterBorder, SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(FSlateColor(InArgs._BorderColor))
			.Padding(InArgs._BorderThickness)
			[
				// 안쪽: 상태색 배경 + 콘텐츠
				SAssignNew(FillBorder, SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(this, &SLB_SquareBox::GetFillColor)
				.Padding(0.f)
				[
					InArgs._Content.Widget
				]
			]
		]
	];
}

void SLB_SquareBox::ApplyStyle(const FLB_SquareBoxStyle& InStyle)
{
	NormalColor = InStyle.NormalColor;
	HoverColor = InStyle.HoverColor;
	PressedColor = InStyle.PressedColor;
	SelectedColor = InStyle.SelectedColor;
	HighlightColor = InStyle.HighlightColor;

	if (SizeBox.IsValid())
	{
		SizeBox->SetWidthOverride(FOptionalSize(static_cast<float>(InStyle.BoxSize.X)));
		SizeBox->SetHeightOverride(FOptionalSize(static_cast<float>(InStyle.BoxSize.Y)));
	}
	if (OuterBorder.IsValid())
	{
		OuterBorder->SetBorderBackgroundColor(FSlateColor(InStyle.BorderColor));
		OuterBorder->SetPadding(FMargin(InStyle.BorderThickness));
	}
}

void SLB_SquareBox::SetContent(const TSharedRef<SWidget>& InContent)
{
	if (FillBorder.IsValid())
	{
		FillBorder->SetContent(InContent);
	}
}



FSlateColor SLB_SquareBox::GetFillColor() const
{
	if (bHighlighted)                    return HighlightColor;
	if (bPressed)                        return PressedColor;
	if (SelectedAttr.Get(false))         return SelectedColor;
	if (IsHovered())                     return HoverColor;
	return NormalColor;
}
