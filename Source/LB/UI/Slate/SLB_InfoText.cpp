#include "SLB_InfoText.h"
#include "SLB_GlowIcon.h"
#include "Components/HorizontalBox.h"
#include "Widgets/Text/STextBlock.h"

void SLB_InfoText::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew(Icon, SLB_GlowIcon)
			.IconText(InArgs._IconText)
			.IconColor(InArgs._IconColor)
			.IconFontSize(InArgs._IconFontSize)   // 추가
			.GlowBoxSize(FVector2D(InArgs._IconBoxSize, InArgs._IconBoxSize))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(10.0f, 0.0f, 0.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			// ↓ 텍스트 묶음을 박스로 감쌈
			SAssignNew(TextBoxBorder, SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(InArgs._BoxColor)
			.Padding(InArgs._BoxPadding)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(LabelTextBlock, STextBlock)
					.Text(InArgs._LabelText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", InArgs._LabelFontSize))
					.ColorAndOpacity(InArgs._LabelColor)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ValueTextBlock, STextBlock)
					.Text(InArgs._ValueText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", InArgs._ValueFontSize))
					.ColorAndOpacity(InArgs._ValueColor)
				]
			]
		]
	];
}
void SLB_InfoText::SetValue(FText InValue)
{
	if (ValueTextBlock.IsValid())
	{
		ValueTextBlock->SetText(InValue);
	}
}

void SLB_InfoText::SetLabelText(FText InLabel)
{
	if (LabelTextBlock.IsValid()) LabelTextBlock->SetText(InLabel);
}

void SLB_InfoText::SetLabelFontSize(int32 InSize)
{
	if (LabelTextBlock.IsValid())
		LabelTextBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(1, InSize)));
}

void SLB_InfoText::SetValueFontSize(int32 InSize)
{
	if (ValueTextBlock.IsValid())
		ValueTextBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(1, InSize)));
}

void SLB_InfoText::SetIconBoxSize(float InSize)
{
	if (Icon.IsValid()) Icon->SetBoxSize(FVector2D(InSize, InSize)); // SetBoxSize는 이미 Layout 무효화함
}

void SLB_InfoText::SetLabelColor(FSlateColor InColor)
{
	if (LabelTextBlock.IsValid()) LabelTextBlock->SetColorAndOpacity(InColor);
}

void SLB_InfoText::SetValueColor(FSlateColor InColor)
{
	if (ValueTextBlock.IsValid()) ValueTextBlock->SetColorAndOpacity(InColor);
}

void SLB_InfoText::SetBoxColor(FSlateColor InColor)
{
	if (TextBoxBorder.IsValid()) TextBoxBorder->SetBorderBackgroundColor(InColor);
}

void SLB_InfoText::SetBoxPadding(FMargin InPadding)
{
	if (TextBoxBorder.IsValid()) TextBoxBorder->SetPadding(InPadding);
}

void SLB_InfoText::SetIconFontSize(int32 InSize)
{
	if (Icon.IsValid()) Icon->SetIconFontSize(InSize);
}


